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
  xLoRaRxQueue = xQueueCreate(10, sizeof(lib::lora::rx_LoRa_msg_t));
  xLoRaTxQueue = xQueueCreate(10, sizeof(lib::lora::tx_LoRa_msg_t));

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
  lib::lora::rx_LoRa_msg_t rx_msg;

  lib::packet::A3MESH_header_t header;
  lib::packet::A3MESH_packet_t pkt;

  bool should_forward;
  bool is_for_me;
};

MeshRxState handleValidate(a3mesh_rx_ctx_t &ctx)
{
  bool ok = lib::packet::A3MESHPacket::deserialize_pkt(
      ctx.rx_msg.rx_data,
      ctx.rx_msg.len,
      ctx.header,
      ctx.pkt);

  if (!ok)
    return MeshRxState::k_rx_drop;

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

  Serial.println("[A3MESH] Packet validated successfully");
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
  neighbor.rssi = ctx.rx_msg.rssi;
  neighbor.last_seen = millis();

  a3mesh.update_neighbor(neighbor);

  return MeshRxState::k_rx_done;
}

MeshRxState handleData(a3mesh_rx_ctx_t &ctx)
{
  ctx.is_for_me =
      (ctx.pkt.dst == a3mesh.get_node_id()) ||
      (ctx.pkt.dst == 0xFFFF);

  if (ctx.is_for_me)
    return MeshRxState::k_rx_local_delivery;

  ctx.should_forward = a3mesh.should_forward(ctx.pkt);

  if (ctx.should_forward)
    return MeshRxState::k_rx_forward;

  return MeshRxState::k_rx_drop;
}

MeshRxState handleLocalDelivery(a3mesh_rx_ctx_t &ctx)
{
  char bleBuffer[BLE_BUFFER_SIZE] = {0};

  int len = snprintf(
      bleBuffer,
      sizeof(bleBuffer),
      "+RCV=%hx,%.*s\r\n",
      ctx.pkt.src,
      ctx.pkt.payload_len,
      ctx.pkt.payload);

  if (len <= 0)
    return MeshRxState::k_rx_drop;

  xQueueSend(xBLETxQueue, bleBuffer, 0);

  return MeshRxState::k_rx_done;
}

MeshRxState handleForward(a3mesh_rx_ctx_t &ctx)
{
  lib::lora::tx_LoRa_msg_t tx_msg;

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
  a3mesh_rx_ctx_t ctx;

  for (;;)
  {
    if (xQueueReceive(xLoRaRxQueue, &ctx.rx_msg, portMAX_DELAY) != pdTRUE)
      continue;

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
  char rx_buffer[BLE_BUFFER_SIZE] = {0};

  for (;;)
  {
    if (xQueueReceive(xBLERxQueue, &rx_buffer, portMAX_DELAY) == pdTRUE)
    {
      if (!BLE_MSG_STARTS_WITH(rx_buffer, "AT"))
      {
        xQueueSend(xBLETxQueue, "+ERR=1\r\n", 0);
        continue;
      }


      if (strcmp(rx_buffer, "AT\r\n") == 0)
      {
        /*
          Expected send format: AT\r\n
        */
        xQueueSend(xBLETxQueue, "+OK\r\n", 0);
        continue;
      }
      else if (BLE_MSG_STARTS_WITH(rx_buffer, "AT+SEND="))
      {
        /*
          Expected send format: AT+SEND=<dst>,<payload>\r\n
        */
        uint16_t dst = 0;
        char msg[A3MESH_MAX_PAYLOAD+1] = {0};

        if (sscanf(rx_buffer + 8, 
                    "%hx,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]", 
                    &dst, 
                    msg) != 2)
        {
          xQueueSend(xBLETxQueue, "+ERR=3\r\n", 0);
          continue;
        }

        lib::packet::A3MESH_header_t header = {
            .version = A3M_VERSION,
            .type = lib::packet::a3mesh_msg_type_t::k_data_type,
            .packet_length = 0 // Will be set in serializeLoRaMessage
        };

        lib::packet::A3MESH_packet_t pkt = {
            .pkt_id = a3mesh.get_next_pkt_id(),
            .src = a3mesh.get_node_id(),
            .node_id = a3mesh.get_node_id(),
            .dst = dst,
            .ttl = MAX_TTL,
            .type = 0, // provisional field
            .flags = (a3mesh.is_neighbor(dst)),
            .payload_len = (uint8_t)strlen(msg)
        };

        memcpy(pkt.payload, msg, pkt.payload_len);

        lib::lora::tx_LoRa_msg_t tx_msg;

        tx_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(
            header,
            pkt,
            tx_msg.tx_data);

        if (xQueueSend(xLoRaTxQueue, &tx_msg, 0) == pdTRUE)
        {
          xQueueSend(xBLETxQueue, "+OK\r\n", 0);
        }
        else
        {
          xQueueSend(xBLETxQueue, "+ERR=4\r\n", 0);
        }
      }
      // else if (strcmp(rx_buffer, "AT+NEIGHBORS?\r\n") == 0)
      // {
      //   uint8_t neighbor_count = a3mesh.get_neighbor_count();
      //   char response[BLE_BUFFER_SIZE] = {0};

      //   int offset = snprintf(response, sizeof(response), "+NEIGHBORS=%d", neighbor_count);

      //   for (uint8_t i = 0; i < neighbor_count && offset < sizeof(response); i++)
      //   {
      //     uint16_t neighbor_id = a3mesh.get_best_neighbor_id(); // This is just an example. You might want to implement a method to get all neighbors.
      //     offset += snprintf(response + offset, sizeof(response) - offset, ",%hx", neighbor_id);
      //   }

      //   offset += snprintf(response + offset, sizeof(response) - offset, "\r\n");

      //   xQueueSend(xBLETxQueue, response, 0);
      // }
      else if (strcmp(rx_buffer, "AT+NODEID?\r\n") == 0)
      {
        char response[BLE_BUFFER_SIZE] = {0};
        snprintf(response, sizeof(response), "+NODEID=%hx\r\n", a3mesh.get_node_id());
        xQueueSend(xBLETxQueue, response, 0);
      }
      else
      {
        xQueueSend(xBLETxQueue, "+ERR=2\r\n", 0);
      }
    }
  }
}

void vSendDiscoveryTask(void *pvParameters)
{
  // Create discovery packet msg
  lib::packet::A3MESH_header_t header = {
      .version = A3M_VERSION,
      .type = lib::packet::a3mesh_msg_type_t::k_discovery_type,
  };

  lib::packet::A3MESH_packet_t pkt = {
      .pkt_id = a3mesh.get_next_pkt_id(),
      .src = a3mesh.get_node_id(),
      .node_id = a3mesh.get_node_id(),
      .dst = 0xFFFF,
      .ttl = 0x01,
      .type = 0, // provisional field
      .flags = 0x00,
      .payload_len = 4
  };

  memcpy(pkt.payload, "DISC", pkt.payload_len);
  
  lib::lora::tx_LoRa_msg_t discovery_msg;
  discovery_msg.len = lib::packet::A3MESHPacket::serialize_header_pkt(
      header,
      pkt,
      discovery_msg.tx_data);
  for (;;)
  {
    xQueueSend(xLoRaTxQueue, &discovery_msg, 0);
    vTaskDelay(pdMS_TO_TICKS(30000) + (pdMS_TO_TICKS(rand() % 30000)));
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


  // --------- Init Link Tasks ---------
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