#include <Arduino.h>
#include "LoRaDriver.h"
#include "BLEDriver.h"

lib::lora::LoRaDriver lora;
lib::ble::BLEDriver ble;



QueueHandle_t queue = nullptr;

void routerTask(void *pvParameters)
{
  lib::packet::A3MESHQueueItem_t item;

  for (;;)
  {
    if (xQueueReceive(queue, &item, portMAX_DELAY) == pdTRUE)
    {
      if (&ble != nullptr && item.src == lib::packet::Transport::LORA)
      {
        xQueueSend(ble.getTxQueue(), &item.pkt, 0);
      }
      if (&lora != nullptr && item.src == lib::packet::Transport::BLE)
      {
        xQueueSend(lora.getTxQueue(), &item.pkt, 0);
      }
    }
  }
}

bool setupA3MESH()
{
  // --------- Init LoRa Driver ---------
  if (!lora.begin(lib::lora::A3MESHConfig()))
  {
    return false;
  }
  Serial.println("[LoRa] init success!");
  lora.setPacketReceivedAction();

  if (!lora.startReceive())
  {
    return false;
  }
  Serial.println("[LoRa] receive mode started!");

  if (!lora.startTasks())
  {
    return false;
  }
  Serial.println("[LoRa] tasks started!");
  // --------- Init BLE Driver ---------
  if (!ble.setup())
  {
    return false;
  }
  Serial.println("[BLE] init success!");
  if (!ble.startTasks())
  {
    return false;
  }
  Serial.println("[BLE] tasks started!");

  queue = xQueueCreate(10, sizeof(lib::packet::A3MESHQueueItem_t));
  if (!queue)
  {
    return false;
  }
  Serial.println("[SYS] router queue created!");

  ble.attachRouterQueue(queue);
  lora.attachRouterQueue(queue);

  xTaskCreate(routerTask, "MeshRouter", 4096, NULL, 2, NULL);
  Serial.println("[SYS] router task created!");
  return true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("[SYS] initBoard");

  delay(1000);
  Serial.println("\n\n\n");
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
    Serial.println("[LoRa] init failed");
    while (1)
      ;
  }
}

void loop()
{
}