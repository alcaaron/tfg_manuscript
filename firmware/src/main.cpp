#include <Arduino.h>
#include "A3MESHManager.h"
#include "LoRaTasks.h"
#include "BLETasks.h"
#include "A3MESHManagerTasks.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define BLE_MSG_STARTS_WITH(str, prefix) (strncmp(str, prefix, strlen(prefix)) == 0)

lib::ble::BLEDriver ble;
lib::lora::LoRaDriver lora;
lib::mesh::A3MESHManager a3mesh;


// ======================
// === Task Handles =====
// ======================

TaskHandle_t txBLETaskHandle;
TaskHandle_t rxLoRaTaskHandle;
TaskHandle_t txLoRaTaskHandle;


// ======================
// ====== Queues ========
// ======================

QueueHandle_t rxBLEQueue;
QueueHandle_t txBLEQueue;

QueueHandle_t rxLoRaQueue;
QueueHandle_t txLoRaQueue;


// ===============================
// ===== BLE Tasks & Queues ======
// ===============================

bool initBLEQueues()
{
  rxBLEQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));
  txBLEQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));

  return rxBLEQueue != NULL && txBLEQueue != NULL;
}

bool startBLETasks()
{
  if (xTaskCreatePinnedToCore(BLEtxTask, "BLE TX Task", 4096, NULL, 1, &txBLETaskHandle, 1) != pdPASS) return false;

  if (xTaskCreatePinnedToCore(lifeBitTask, "BLE Life Bit Task", 4096, NULL, 1, NULL, 1) != pdPASS) return false;

  return true;
}


// ================================
// ===== LoRa Tasks & Queues ======
// ================================

bool initLoRaQueues()
{
  rxLoRaQueue = xQueueCreate(10, sizeof(lib::lora::RxMessage_t));
  txLoRaQueue = xQueueCreate(10, sizeof(lib::lora::TxMessage_t));

  return rxLoRaQueue != NULL && txLoRaQueue != NULL;
}

bool startLoRaTasks()
{
  if(xTaskCreatePinnedToCore(rxLoRaPacketTask, "LoRa RX", 4096, NULL, 1, &rxLoRaTaskHandle, 0) != pdPASS) return false;

  if(xTaskCreatePinnedToCore(txLoRaPacketTask, "LoRa TX", 4096, NULL, 1, &txLoRaTaskHandle, 0) != pdPASS) return false;

  return true;
}


// =================================
// ===== Main Application Logic ====
// =================================

enum class MeshRxState
{
  RX_IDLE,

  RX_VALIDATE,
  RX_CLASSIFY,

  RX_PROCESS_DISCOVERY,
  RX_PROCESS_DATA,

  RX_LOCAL_DELIVERY,
  RX_FORWARD,

  RX_DROP,
  RX_DONE
};

struct MeshRxContext
{
  lib::lora::RxMessage_t rxMsg;

  lib::packet::A3MESHHeader_t header;
  lib::packet::A3MESHPacket_t pkt;

  bool shouldForward;
  bool isForMe;
};

MeshRxState handleValidate(MeshRxContext &ctx)
{
  bool ok = lib::packet::A3MESHPacket::deserializePacket(
      ctx.rxMsg.rx_data,
      ctx.rxMsg.len,
      ctx.header,
      ctx.pkt);

  if (!ok)
    return MeshRxState::RX_DROP;

  if (ctx.pkt.src == a3mesh.getNodeID())
    return MeshRxState::RX_DROP;

  if (ctx.pkt.node_id == a3mesh.getNodeID())
    return MeshRxState::RX_DROP;

  if (ctx.pkt.ttl == 0)
    return MeshRxState::RX_DROP;

  if (a3mesh.isDuplicate(ctx.pkt))
    return MeshRxState::RX_DROP;

  return MeshRxState::RX_CLASSIFY;
}

MeshRxState handleClassify(MeshRxContext &ctx)
{
  switch (ctx.header.type)
  {
  case lib::packet::MSG_TYPE_DISCOVERY:
    return MeshRxState::RX_PROCESS_DISCOVERY;

  case lib::packet::MSG_TYPE_DATA:
    return MeshRxState::RX_PROCESS_DATA;

  default:
    return MeshRxState::RX_DROP;
  }
}

MeshRxState handleDiscovery(MeshRxContext &ctx)
{
  lib::mesh::Neighbor neighbor;

  neighbor.nodeID = ctx.pkt.src;
  neighbor.rssi = ctx.rxMsg.rssi;
  neighbor.lastSeen = millis();

  a3mesh.updateNeighbor(neighbor);

  return MeshRxState::RX_DONE;
}

MeshRxState handleData(MeshRxContext &ctx)
{
  ctx.isForMe =
      (ctx.pkt.dst == a3mesh.getNodeID()) ||
      (ctx.pkt.dst == 0xFFFF);

  if (ctx.isForMe)
    return MeshRxState::RX_LOCAL_DELIVERY;

  ctx.shouldForward = a3mesh.shouldForward(ctx.pkt);

  if (ctx.shouldForward)
    return MeshRxState::RX_FORWARD;

  return MeshRxState::RX_DROP;
}

MeshRxState handleLocalDelivery(MeshRxContext &ctx)
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
    return MeshRxState::RX_DROP;

  xQueueSend(txBLEQueue, bleBuffer, 0);

  return MeshRxState::RX_DONE;
}

MeshRxState handleForward(MeshRxContext &ctx)
{
  lib::lora::TxMessage_t txMsg;

  ctx.pkt.pkt_id = a3mesh.getNextMsgID();
  ctx.pkt.node_id = a3mesh.getNodeID();
  ctx.pkt.neighbor_id = (a3mesh.IsNeighbor(ctx.pkt.dst)) ? ctx.pkt.dst : a3mesh.GetBestNeighborID();
  ctx.pkt.ttl--;

  txMsg.len = lib::packet::A3MESHPacket::serializeLoRaMessage(ctx.header, ctx.pkt, txMsg.tx_data);

  vTaskDelay(pdMS_TO_TICKS(rand() % 250));

  xQueueSend(txLoRaQueue, &txMsg, 0);

  return MeshRxState::RX_DONE;
}

void LoRa2BLETask(void *pvParameters)
{
  MeshRxContext ctx;

  for (;;)
  {
    if (xQueueReceive(rxLoRaQueue, &ctx.rxMsg, portMAX_DELAY) != pdTRUE)
      continue;

    MeshRxState state = MeshRxState::RX_VALIDATE;

    while (state != MeshRxState::RX_DONE)
    {
      switch (state)
      {

      case MeshRxState::RX_VALIDATE:
        state = handleValidate(ctx);
        break;

      case MeshRxState::RX_CLASSIFY:
        state = handleClassify(ctx);
        break;

      case MeshRxState::RX_PROCESS_DISCOVERY:
        state = handleDiscovery(ctx);
        break;

      case MeshRxState::RX_PROCESS_DATA:
        state = handleData(ctx);
        break;

      case MeshRxState::RX_LOCAL_DELIVERY:
        state = handleLocalDelivery(ctx);
        break;

      case MeshRxState::RX_FORWARD:
        state = handleForward(ctx);
        break;

      case MeshRxState::RX_DROP:
        state = MeshRxState::RX_DONE;
        break;

      default:
        state = MeshRxState::RX_DONE;
        break;
      }
    }
  }
}

void BLE2LoRaTask(void *pvParameters)
{
  char rxBuffer[BLE_BUFFER_SIZE] = {0};

  for (;;)
  {
    if (xQueueReceive(rxBLEQueue, &rxBuffer, portMAX_DELAY) == pdTRUE)
    {
      if (!BLE_MSG_STARTS_WITH(rxBuffer, "AT"))
      {
        xQueueSend(txBLEQueue, "+ERR=1\r\n", 0);
        continue;
      }
      if (strcmp(rxBuffer, "AT\r\n") == 0)
      {
        xQueueSend(txBLEQueue, "+OK\r\n", 0);
        continue;
      }
      else if (BLE_MSG_STARTS_WITH(rxBuffer, "AT+SEND="))
      {
        /*
          Expected send format: AT+SEND=<dst>,<payload>\r\n
        */
        uint16_t dst = 0;
        char msg[A3MESH_MAX_PAYLOAD+1] = {0};

        if (sscanf(rxBuffer + 8, 
                    "%hx,%" STR(A3MESH_MAX_PAYLOAD) "[^\r\n]", 
                    &dst, 
                    msg) != 2)
        {
          xQueueSend(txBLEQueue, "+ERR=3\r\n", 0);
          continue;
        }

        lib::packet::A3MESHHeader_t header = {
            .version = A3M_VERSION,
            .type = lib::packet::MessageType::MSG_TYPE_DATA,
        };

        lib::packet::A3MESHPacket_t pkt = {
            .pkt_id = a3mesh.getNextMsgID(),
            .node_id = a3mesh.getNodeID(),
            .src = a3mesh.getNodeID(),
            .dst = dst,
            .neighbor_id = (a3mesh.IsNeighbor(dst)) ? dst : a3mesh.GetBestNeighborID(),
            .ttl = MAX_TTL,
            .flags = 0x00,
            .payload_len = (uint8_t)strlen(msg)
        };

        memcpy(pkt.payload, msg, pkt.payload_len);

        lib::lora::TxMessage_t txMsg;

        txMsg.len = lib::packet::A3MESHPacket::serializeLoRaMessage(
            header,
            pkt,
            txMsg.tx_data);

        if (xQueueSend(txLoRaQueue, &txMsg, 0) == pdTRUE)
        {
          xQueueSend(txBLEQueue, "+OK\r\n", 0);
        }
        else
        {
          xQueueSend(txBLEQueue, "+ERR=4\r\n", 0);
        }
      }
      else
      {
        xQueueSend(txBLEQueue, "+ERR=2\r\n", 0);
      }
    }
  }
}

void sendDiscoveryTask(void *pvParameters)
{
  // Create discovery packet msg
  lib::packet::A3MESHHeader_t header = {
      .version = A3M_VERSION,
      .type = lib::packet::MessageType::MSG_TYPE_DISCOVERY,
  };

  lib::packet::A3MESHPacket_t pkt = {
      .pkt_id = a3mesh.getNextMsgID(),
      .node_id = a3mesh.getNodeID(),
      .src = a3mesh.getNodeID(),
      .dst = 0xFFFF,
      .neighbor_id = 0xFFFF,
      .ttl = 0x01,
      .flags = 0x00,
      .payload_len = 4
  };

  memcpy(pkt.payload, "DISC", pkt.payload_len);
  
  lib::lora::TxMessage_t discoveryMsg;
  discoveryMsg.len = lib::packet::A3MESHPacket::serializeLoRaMessage(
      header,
      pkt,
      discoveryMsg.tx_data);
  for (;;)
  {
    xQueueSend(txLoRaQueue, &discoveryMsg, 0);
    vTaskDelay(pdMS_TO_TICKS(30000) + (pdMS_TO_TICKS(rand() % 30000)));
  }
  vTaskDelete(NULL);
}

bool setupA3MESH()
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

  if (!initBLEQueues()) return false;
  Serial.println("[BLE] queues initialized!");

  if (!startBLETasks()) return false;
  Serial.println("[BLE] tasks started!");


  // --------- Init Link Tasks ---------
  if (xTaskCreatePinnedToCore(BLE2LoRaTask, "BLE2LoRaTask", 4096, NULL, 1, NULL, 1) != pdPASS) return false;
  Serial.println("[SYS] BLE2LORA started!");

  if (xTaskCreatePinnedToCore(LoRa2BLETask, "LoRa2BLETask", 4096, NULL, 1, NULL, 1) != pdPASS) return false;
  Serial.println("[SYS] LORA2BLE started!");


  // --------- Init A3MESH Manager ---------
  if (xTaskCreatePinnedToCore(sendDiscoveryTask, "DiscoveryTask", 4096, NULL, 1, NULL, 0) != pdPASS) return false;
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

  if (!setupA3MESH())
  {
    Serial.println("[SYS] Setup failed");
    while (1)
      ;
  }
}

void loop()
{
}