// ================
// === INCLUDES ===
// ================

#include <Arduino.h>
#include "A3MESHManager.hh"
#include "LoRaTasks.h"
#include "BLETasks.h"
#include "A3MESHManagerTasks.h"


// ======================
// === Global Objects ===
// ======================

lib::ble::BLEDriver ble;
lib::lora::LoRaDriver lora;
lib::mesh::A3MESHManager a3mesh;


// ======================
// === Task Handles =====
// ======================

// TaskHandle_t tx_BLE_task_handle;
// TaskHandle_t rx_LoRa_task_handle;
// TaskHandle_t tx_LoRa_task_handle;


// ======================
// ====== Queues ========
// ======================

QueueHandle_t xBLERxQueue;
QueueHandle_t xBLETxQueue;

QueueHandle_t xLoRaRxQueue;
QueueHandle_t xLoRaTxQueue;


// ===============================
// ===== BLE Tasks & Queues ======
// ===============================

bool init_BLE_queues()
{
  xBLETxQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));
  xBLERxQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));

  return xBLERxQueue != NULL && xBLETxQueue != NULL;
}

bool start_BLE_tasks()
{
  if (xTaskCreatePinnedToCore(vBLETxTask, "BLE TX Task", 4096, NULL, 1, NULL, 1) != pdPASS) return false;

  if (xTaskCreatePinnedToCore(vBLELifeBitTask, "BLE Life Bit Task", 4096, NULL, 1, NULL, 1) != pdPASS) return false;

  return true;
}


// ================================
// ===== LoRa Tasks & Queues ======
// ================================

bool initLoRaQueues()
{
  xLoRaRxQueue = xQueueCreate(10, sizeof(lib::lora::LoRaDriver::rx_LoRa_msg_t*));
  xLoRaTxQueue = xQueueCreate(10, sizeof(lib::lora::LoRaDriver::tx_LoRa_msg_t));

  return xLoRaRxQueue != NULL && xLoRaTxQueue != NULL;
}

bool startLoRaTasks()
{
  if(xTaskCreatePinnedToCore(vLoRaRxTask, "LoRa RX", 4096, NULL, 1, NULL, 0) != pdPASS) return false;

  if(xTaskCreatePinnedToCore(vLoRaTxTask, "LoRa TX", 4096, NULL, 1, NULL, 0) != pdPASS) return false;

  return true;
}

// =================================
// ===== Main Application Logic ====
// =================================

enum class MeshRxState
{
  k_rx_idle,

  k_rx_validate,
  k_rx_classify,

  k_rx_process_discovery,
  k_rx_process_data,

  k_rx_local_delivery,
  k_rx_forward,

  k_rx_drop,
  k_rx_done
};

struct a3mesh_rx_ctx_t
{
  lib::lora::LoRaDriver::rx_LoRa_msg_t* rx_msg;

  lib::packet::A3MESH_header_t header;
  lib::packet::A3MESH_packet_t pkt;

  bool should_forward;
  bool is_for_me;
};

MeshRxState handleValidate(a3mesh_rx_ctx_t &ctx)
{
  int8_t ok = lib::packet::A3MESHPacket::deserialize_pkt(
      ctx.rx_msg->rx_data,
      ctx.rx_msg->len,
      ctx.header,
      ctx.pkt);

  if (ok < 0){
    Serial.printf("[A3MESH] Failed to deserialize packet, e: %d\r\n", ok);
    return MeshRxState::k_rx_drop;
  }

  Serial.print("[A3MESH] ReceivedHeader:");
  Serial.printf("[A3MESH] version: %u, type: %u, packet_length: %u\r\n",
      ctx.header.version,
      ctx.header.type,
      ctx.header.packet_length);

  Serial.print("[A3MESH] Received packet:");
  Serial.printf("[A3MESH] pkt_id: %u, src: %hx, node_id: %hx, dst: %hx, ttl: %u, type: %u, flags: %u, payload_len: %u\r\n",
      ctx.pkt.pkt_id,
      ctx.pkt.src,
      ctx.pkt.node_id,
      ctx.pkt.dst,
      ctx.pkt.ttl,
      ctx.pkt.type,
      ctx.pkt.flags,
      ctx.pkt.payload_len);

  if (ctx.pkt.src == a3mesh.get_node_id())
    return MeshRxState::k_rx_drop;

  if (ctx.pkt.node_id == a3mesh.get_node_id())
    return MeshRxState::k_rx_drop;

  if (ctx.pkt.ttl == 0)
    return MeshRxState::k_rx_drop;

  if (a3mesh.is_duplicate(ctx.pkt))
    return MeshRxState::k_rx_drop;

  // Serial.println("[A3MESH] Packet validated successfully");
  return MeshRxState::k_rx_classify;
}

MeshRxState handleClassify(a3mesh_rx_ctx_t &ctx)
{
  switch (ctx.header.type)
  {
  case lib::packet::k_discovery_type:
    return MeshRxState::k_rx_process_discovery;

  case lib::packet::k_data_type:
    return MeshRxState::k_rx_process_data;

  default:
    return MeshRxState::k_rx_drop;
  }
}

MeshRxState handleDiscovery(a3mesh_rx_ctx_t &ctx)
{
  lib::mesh::Neighbor neighbor;

  neighbor.node_id = ctx.pkt.src;
  neighbor.rssi = ctx.rx_msg->rssi;
  neighbor.last_seen = millis();

  a3mesh.update_neighbor(neighbor);

  return MeshRxState::k_rx_done;
}

MeshRxState handleData(a3mesh_rx_ctx_t &ctx)
{
  // Serial.println("[A3MESH] Processing data packet");
  ctx.is_for_me =
      (ctx.pkt.dst == a3mesh.get_node_id()) ||
      (ctx.pkt.dst == 0xFFFF);

  if (ctx.is_for_me)
    return MeshRxState::k_rx_local_delivery;

  // Serial.println("[A3MESH] Packet not for me, checking if it should be forwarded");
  ctx.should_forward = a3mesh.should_forward(ctx.pkt);

  if (ctx.should_forward)
    return MeshRxState::k_rx_forward;

  return MeshRxState::k_rx_drop;
}

MeshRxState handleLocalDelivery(a3mesh_rx_ctx_t &ctx)
{
  // Serial.println("[A3MESH] Delivering packet locally");
  char bleBuffer[BLE_BUFFER_SIZE] = {0};

  int len = snprintf(
      bleBuffer,
      sizeof(bleBuffer),
      "+RCV=%hx,%hx,%.*s\r\n",
      ctx.pkt.src,
      ctx.pkt.node_id,
      ctx.pkt.payload_len,
      ctx.pkt.payload);

  if (len <= 0)
    return MeshRxState::k_rx_drop;

  xQueueSend(xBLETxQueue, bleBuffer, 0);

  return MeshRxState::k_rx_done;
}

MeshRxState handleForward(a3mesh_rx_ctx_t &ctx)
{
  lib::lora::LoRaDriver::tx_LoRa_msg_t tx_msg;

  ctx.pkt.pkt_id = a3mesh.get_next_pkt_id();
  ctx.pkt.node_id = a3mesh.get_node_id();
  ctx.pkt.ttl--;
  ctx.pkt.flags = (a3mesh.is_neighbor(ctx.pkt.dst));

  tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(ctx.header, ctx.pkt, tx_msg.tx_data);

  vTaskDelay(pdMS_TO_TICKS(rand() % 250));

  xQueueSend(xLoRaTxQueue, &tx_msg, 0);

  return MeshRxState::k_rx_done;
}

void LoRa2BLETask(void *pvParameters)
{
  lib::lora::LoRaDriver::rx_LoRa_msg_t* frame;

  for (;;)
  {
    if (xQueueReceive(xLoRaRxQueue, &frame, portMAX_DELAY) != pdTRUE)
      continue;

    a3mesh_rx_ctx_t ctx ;
    ctx.rx_msg = frame;

    MeshRxState state = MeshRxState::k_rx_validate;

    while (state != MeshRxState::k_rx_done)
    {
      switch (state)
      {

      case MeshRxState::k_rx_validate:
        state = handleValidate(ctx);
        break;

      case MeshRxState::k_rx_classify:
        state = handleClassify(ctx);
        break;

      case MeshRxState::k_rx_process_discovery:
        state = handleDiscovery(ctx);
        break;

      case MeshRxState::k_rx_process_data:
        state = handleData(ctx);
        break;

      case MeshRxState::k_rx_local_delivery:
        state = handleLocalDelivery(ctx);
        break;

      case MeshRxState::k_rx_forward:
        state = handleForward(ctx);
        break;

      case MeshRxState::k_rx_drop:
        state = MeshRxState::k_rx_done;
        break;

      default:
        state = MeshRxState::k_rx_done;
        break;
      }
    }
  }
}

void BLE2LoRaTask(void *pvParameters)
{
  char rx_buffer[BLE_BUFFER_SIZE];

  const uint32_t node_id = a3mesh.get_node_id();

  lib::packet::A3MESH_header_t header = {
      .version = A3M_VERSION,
      .type = lib::packet::a3mesh_msg_type_t::k_data_type,
  };

  lib::packet::A3MESH_packet_t pkt = {
      .src = node_id,
      .node_id = node_id,
      .ttl = MAX_TTL,
      .type = 0x00
  };

  lib::lora::LoRaDriver::tx_LoRa_msg_t tx_msg = {};

  for (;;)
  {
    if (xQueueReceive(xBLERxQueue, &rx_buffer, portMAX_DELAY) != pdTRUE)
      continue;

    // -------------------------
    // AT basic validation
    // -------------------------
    if (!BLE_MSG_STARTS_WITH(rx_buffer, "AT"))
    {
      xQueueSend(xBLETxQueue, "+ERR=1\r\n", 0);
      continue;
    }


    // -------------------------
    // AT command
    // -------------------------
    if (strcmp(rx_buffer, "AT\r\n") == 0)
    {
      /*
        Expected send format: AT\r\n
      */
      xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      continue;
    }


    // -------------------------
    // SEND command
    // Format: AT+SEND=<dst>,<payload>\r\n
    // -------------------------
    if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SEND="))
    {
      uint32_t dst = 0;

      char msg[A3MESH_MAX_PAYLOAD+1];

      const char *args = rx_buffer + 8;

      // memset(pkt.payload, 0, sizeof(pkt.payload));

      if (sscanf(args, 
                  "%hx,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]", 
                  &dst, 
                  msg) != 2)
      {
        xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
        continue;
      }

      // -------------------------
      // Packet fill (dynamic fields only)
      // -------------------------
      pkt.pkt_id = a3mesh.get_next_pkt_id();
      pkt.dst = dst;
      pkt.flags = (a3mesh.is_neighbor(dst)) ? 0x01 : 0x00;
      pkt.payload_len = (uint8_t)strlen(msg);

      memcpy(pkt.payload, msg, pkt.payload_len);

      // -------------------------
      // Serialize
      // -------------------------
      tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(
          header,
          pkt,
          tx_msg.tx_data);

      // -------------------------
      // TX queue
      // -------------------------
      Serial.println("[BLE2LORA] Sending header:");
      Serial.printf("[BLE2LORA] version: %u, type: %u, packet_length: %u\r\n",
          header.version,
          header.type,
          header.packet_length);
      Serial.printf("[BLE2LORA] Sending packet:\r\n");
      Serial.printf("[BLE2LORA] pkt_id: %u, src: %hx, node_id: %hx, dst: %hx, ttl: %u, type: %u, flags: %u, payload_len: %u, payload: %.*s\r\n",
          pkt.pkt_id,
          pkt.src,
          pkt.node_id,
          pkt.dst,
          pkt.ttl,
          pkt.type,
          pkt.flags,
          pkt.payload_len,
          pkt.payload_len,
          pkt.payload);
      if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
      {
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
      }
      else
      {
        xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
      }
      continue;
    }


    // -------------------------
    // NEIGHBORS
    // -------------------------
    if (strcmp(rx_buffer, "AT+NEIGHBORS?\r\n") == 0)
    {
      char response[BLE_BUFFER_SIZE];
      char neighbors_str[BLE_BUFFER_SIZE];

      lib::mesh::Neighbor neighbors_list[MAX_NEIGHBORS];
      const uint8_t count = a3mesh.get_neighbor_count();

      if (count == 0)
      {
        snprintf(response,
          sizeof(response),
          "+NEIGHBORS=0\r\n+OK\r\n");

        xQueueSend(xBLETxQueue, response, 0);
        continue;
      }

      a3mesh.get_neighbors(neighbors_list);

      int offset = 0;

      for (uint8_t i = 0; i < count; i++)
      {
        offset += snprintf(
            neighbors_str + offset,
            sizeof(neighbors_str) - offset,
            "+N:%hx,RSSI:%d,T:%lu;\r\n",
            neighbors_list[i].node_id,
            neighbors_list[i].rssi,
            neighbors_list[i].last_seen);

        if (offset >= sizeof(neighbors_str))
          break;
      }

      snprintf(response,
        sizeof(response),
        "+NEIGHBORS=%d\r\n%s\r\n+OK\r\n",
        count,
        neighbors_str);

      xQueueSend(xBLETxQueue, response, 0);
      continue;
    }


    // -------------------------
    // NODE ID
    // -------------------------
    if (strcmp(rx_buffer, "AT+NODEID?\r\n") == 0)
    {
      char response[BLE_BUFFER_SIZE];

      snprintf(response,
        sizeof(response),
        "+NODEID=%hx\r\n",
        node_id);

      xQueueSend(xBLETxQueue, response, 0);
      continue;
    }


    // -------------------------
    // Fallback
    // -------------------------
    xQueueSend(xBLETxQueue, "+ERR=2\r\n", 0);
  }
}

void vSendDiscoveryTask(void *pvParameters)
{
  const uint32_t node_id = a3mesh.get_node_id();
  // Create discovery packet msg
  lib::packet::A3MESH_header_t header = {
      .version = A3M_VERSION,
      .type = lib::packet::a3mesh_msg_type_t::k_discovery_type,
  };

  lib::packet::A3MESH_packet_t pkt = {
      .pkt_id = a3mesh.get_next_pkt_id(),
      .src = node_id,
      .node_id = node_id,
      .dst = 0xFFFFFFFF, // Broadcast
      .ttl = 0x01,
      .type = 0x01, // provisional field
      .flags = 0x00,
      .payload_len = 4
  };

  memcpy(pkt.payload, "DISC", pkt.payload_len);
  
  lib::lora::LoRaDriver::tx_LoRa_msg_t discovery_msg;
  discovery_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(
      header,
      pkt,
      discovery_msg.tx_data);

  for (;;)
  {
    uint16_t pkt_id = a3mesh.get_next_pkt_id();
    discovery_msg.tx_data[4] = pkt_id & 0xFF;
    discovery_msg.tx_data[5] = (pkt_id >> 8) & 0xFF;
    xQueueSend(xLoRaTxQueue, &discovery_msg, 0);
    vTaskDelay(pdMS_TO_TICKS(DISCOVERY_INTERVAL_MS) + (pdMS_TO_TICKS(rand() % 10000)));
  }
  vTaskDelete(NULL);
}

bool setup_A3MESH()
{
  // --------- Init SPI ---------
  SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);


  // --------- Init LoRa Driver ---------
  if (!lora.begin(lib::lora::A3MESHConfig())) return false;
  Serial.println("[LORA] init success!");

  if (!initLoRaQueues()) return false;
  Serial.println("[LORA] queues initialized!");

  if (!startLoRaTasks()) return false;
  Serial.println("[LORA] tasks started!");


  // --------- Init BLE Driver ---------
  if (!ble.setup()) return false;
  Serial.println("[BLE] init success!");

  if (!init_BLE_queues()) return false;
  Serial.println("[BLE] queues initialized!");

  if (!start_BLE_tasks()) return false;
  Serial.println("[BLE] tasks started!");


  // --------- Init Core Tasks ---------
  if (xTaskCreatePinnedToCore(BLE2LoRaTask, "BLE2LoRaTask", 4096, NULL, 1, NULL, 1) != pdPASS) return false;
  Serial.println("[SYS] BLE2LORA started!");

  if (xTaskCreatePinnedToCore(LoRa2BLETask, "LoRa2BLETask", 4096, NULL, 1, NULL, 1) != pdPASS) return false;
  Serial.println("[SYS] LORA2BLE started!");


  // --------- Init A3MESH Manager ---------
  if (xTaskCreatePinnedToCore(vSendDiscoveryTask, "DiscoveryTask", 4096, NULL, 1, NULL, 0) != pdPASS) return false;
  Serial.println("[MESH] Discovery Task Initialized!");

  if (xTaskCreatePinnedToCore(vNeighborCheckTask, "NeighborCheckTask", 4096, NULL, 1, NULL, 0) != pdPASS) return false;
  Serial.println("[MESH] Neighbor Check Task Initialized!");
  return true;
}

void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println("[SYS] initBoard");

  delay(1000);
  switch (esp_reset_reason())
  {
  case ESP_RST_BROWNOUT:
    Serial.println("[SYS] power down detected");
    break;
  case ESP_RST_POWERON:
    Serial.println("[SYS] power on detected");
    break;
  case ESP_RST_WDT:
    Serial.println("[SYS] watch dog timer");
    break;
  case ESP_RST_TASK_WDT:
    Serial.println("[SYS] task WDT");
    break;
  case ESP_RST_PANIC:
    Serial.println("[SYS] self reset");
    break;
  default:
    break;
  }

  if (!setup_A3MESH())
  {
    Serial.println("[SYS] Setup failed");
    while (1)
      ;
  }
}

void loop()
{
}