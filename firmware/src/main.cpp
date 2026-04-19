#include <Arduino.h>
#include "A3MESHManager.h"

lib::lora::LoRaDriver lora;
lib::ble::BLEDriver ble;
lib::mesh::A3MESHManager A3MESHManager;

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

  // --------- Init A3MESH Manager ---------
  if (!A3MESHManager.begin())
  {
    return false;
  }
  Serial.println("[MESH] manager started!");

  A3MESHManager.attachBLE(&ble);
  A3MESHManager.attachLoRa(&lora);
  Serial.println("[MESH] drivers attached to manager!");
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