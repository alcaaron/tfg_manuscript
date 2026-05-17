#include <Arduino.h>
#include "A3MESHManager.h"
#include "LoRaTasks.h"
#include "BLETasks.h"
#include "AMESHManagerTasks.h"

lib::ble::BLEDriver ble;

QueueHandle_t rxBLEQueue;
QueueHandle_t txBLEQueue;

TaskHandle_t txBLETaskHandle;

bool initBLEQueues()
{
  rxBLEQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));
  txBLEQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));

  return rxBLEQueue != NULL && txBLEQueue != NULL;
}

bool startBLETasks()
{
  if (xTaskCreatePinnedToCore(BLEtxTask, "BLE TX Task", 4096, NULL, 1, &txBLETaskHandle, 1) != pdPASS)
  {
    return false;
  }

  if (xTaskCreatePinnedToCore(lifeBitTask, "BLE Life Bit Task", 4096, NULL, 1, NULL, 1) != pdPASS)
  {
    return false;
  }

  return true;
}

lib::lora::LoRaDriver lora;

QueueHandle_t rxLoRaQueue;
QueueHandle_t txLoRaQueue;

TaskHandle_t rxLoRaTaskHandle;
TaskHandle_t txLoRaTaskHandle;

bool initLoRaQueues()
{
  rxLoRaQueue = xQueueCreate(10, sizeof(lib::lora::RxMessage_t));
  txLoRaQueue = xQueueCreate(10, sizeof(lib::lora::TxMessage_t));

  return rxLoRaQueue != NULL && txLoRaQueue != NULL;
}

bool startLoRaTasks()
{

  BaseType_t result = xTaskCreatePinnedToCore( rxLoRaPacketTask, "LoRa RX", 4096, NULL, 1, &rxLoRaTaskHandle, 0);

  if (result != pdPASS)
  {
    return false;
  }

  result = xTaskCreatePinnedToCore( txLoRaPacketTask, "LoRa TX", 4096, NULL, 1, &txLoRaTaskHandle, 0);

  if (result != pdPASS)
  {
    return false;
  }

  return true;
}

lib::mesh::A3MESHManager a3mesh;

void LoRa2BLETask(void *pvParameters)
{
  lib::lora::RxMessage_t rxMsg;

  for (;;)
  {
    if (xQueueReceive(rxLoRaQueue, &rxMsg, portMAX_DELAY) == pdTRUE)
    {
      lib::packet::A3MESHHeader_t header;
      lib::packet::A3MESHPacket_t pkt;

      bool ok = lib::packet::A3MESHPacket::deserializePacket(
          rxMsg.rx_data,
          rxMsg.len,
          header,
          pkt);

      if (!ok)
      {
        Serial.println("[LORA2BLE] Failed to deserialize packet");
        continue;
      }

      // Steps when rx a message
      // x. Discard msg if src matches with node_id
      if (pkt.src == a3mesh.getNodeID()) { continue; }
      // x. Check msg type
      if (header.type == lib::packet::MSG_TYPE_DISCOVERY)
      {
        // Manage neighbors table
        Serial.println("[LORA2BLE] DISC msg-type received!");
      }
      else if (header.type == lib::packet::MSG_TYPE_DATA)
      {
        Serial.println("[MESH] DATA msg-type received!");
        // x. Check dst matches with node_id
        //  --> IF node_id matches THEN forward directly to BLE
        //  --> ELSE route or not uppon routing logic
        if (pkt.dst == a3mesh.getNodeID())
        {
          char bleBuffer[BLE_BUFFER_SIZE] = {0};

          if (pkt.payload_len >= (BLE_BUFFER_SIZE - 5)) { continue; }

          int len = snprintf(
              bleBuffer,
              sizeof(bleBuffer),
              "+RCV=%.*s\r\n",
              pkt.payload_len,
              pkt.payload);

          if (len < 0 || len >= BLE_BUFFER_SIZE) { continue; }

          Serial.printf("[LORA2BLE] Forwarding to BLE: %s\n", bleBuffer);
          xQueueSend(txBLEQueue, bleBuffer, 0);
        }
        else
        {
          // Route the packet if applies
          if (a3mesh.shouldForward(&pkt))
          {
            lib::lora::TxMessage_t txMsg;
            // Keep the rx pkt except:
            // + ttl is decreased by 1
            // + node_id is the current node's
            // + msg_id is changed bc. is a new msg
            lib::packet::A3MESHPacket::createLoRaMessage(
                header.version,
                header.type,
                a3mesh.getNextMsgID(),
                a3mesh.getNodeID(), // node_id
                pkt.src, // src
                pkt.dst,
                pkt.ttl - 1,
                0, // flags
                (const uint8_t *)pkt.payload,
                txMsg.tx_data,
                txMsg.len);

            lora.sendRawPacket(txMsg.tx_data, txMsg.len);
          }
        }
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
      /*
      Expected send format: AT+SEND=<payload>[,<dst>]
      */
      if (strncmp(rxBuffer, "AT+SEND=", 8) == 0)
      {
        char *payload_start = rxBuffer + 8;
        char *dst_str = nullptr;

        char *comma = strchr(payload_start, ',');
        uint16_t dst = 0xFFFF; // default broadcast

        if (comma != nullptr)
        {
          *comma = '\0';
          dst_str = comma + 1;

          dst = (uint16_t)strtol(dst_str, nullptr, 0);
        }

        const char *msg = payload_start;
        size_t msg_len = strlen(msg);

        if (msg_len == 0 || msg_len > A3MESH_MAX_PAYLOAD)
        {
          continue;
        }

        lib::lora::TxMessage_t lora_msg;

        lib::packet::A3MESHPacket::createLoRaMessage(
            A3M_VERSION,
            lib::packet::MessageType::MSG_TYPE_DATA,
            a3mesh.getNextMsgID(),
            a3mesh.getNodeID(), // node_id
            a3mesh.getNodeID(), // src
            dst,
            MAX_TTL,
            0, // flags
            (const uint8_t *)msg,
            lora_msg.tx_data,
            lora_msg.len);

        xQueueSend(txLoRaQueue, &lora_msg, 0);
      }
    }
  }
}

void sendDiscoveryTask(void *pvParameters)
{
    for (;;)
    {
        const char *msg = "DISC";

        lib::lora::TxMessage_t lora_msg;
        lib::packet::A3MESHPacket::createLoRaMessage(
            A3M_VERSION,
            lib::packet::MessageType::MSG_TYPE_DISCOVERY,
            a3mesh.getNextMsgID(),
            a3mesh.getNodeID(),
            a3mesh.getNodeID(),
            0xFFFF, // Broadcast
            1,      // TTL
            0,      // Flags
            (const uint8_t *)msg,
            lora_msg.tx_data,
            lora_msg.len);

        xQueueSend(txLoRaQueue, &lora_msg, 0);
        vTaskDelay(pdMS_TO_TICKS(30000) + (pdMS_TO_TICKS(rand() % 30000)));
    }
    vTaskDelete(NULL);
}

bool setupA3MESH()
{
  // --------- Init SPI ---------
  SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);

  // --------- Init LoRa Driver ---------
  if (!lora.begin(lib::lora::A3MESHConfig()))
  {
    return false;
  }
  Serial.println("[LORA] init success!");

  if (!initLoRaQueues())
  {
    return false;
  }
  Serial.println("[LORA] queues initialized!");

  if (!startLoRaTasks())
  {
    return false;
  }
  Serial.println("[LORA] tasks started!");

  // --------- Init BLE Driver ---------
  if (!ble.setup())
  {
    return false;
  }
  Serial.println("[BLE] init success!");

  if (!initBLEQueues())
  {
    return false;
  }
  Serial.println("[BLE] queues initialized!");

  if (!startBLETasks())
  {
    return false;
  }
  Serial.println("[BLE] tasks started!");

  // --------- Init Link Tasks ---------
  BaseType_t result = xTaskCreatePinnedToCore(BLE2LoRaTask, "BLE2LoRaTask", 4096, NULL, 1, NULL, 1);
  if (result != pdPASS)
  {
    return false;
  }
  result = xTaskCreatePinnedToCore(LoRa2BLETask, "LoRa2BLETask", 4096, NULL, 1, NULL, 1);
  if (result != pdPASS)
  {
    return false;
  }

  // --------- Init A3MESH Manager ---------
  result = xTaskCreatePinnedToCore(sendDiscoveryTask, "DiscoveryTask", 4096, NULL, 1, NULL, 0);
  if (result != pdPASS)
  {
    return false;
  }
  Serial.println("[MESH] Discovery Task Initialized!");

  return true;
}

void setup()
{
  Serial.begin(115200);
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