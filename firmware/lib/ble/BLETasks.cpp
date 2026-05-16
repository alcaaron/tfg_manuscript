#include "BLETasks.h"

extern lib::ble::BLEDriver ble;

void BLEtxTask(void *pvParameters)
{
    char txBuffer[BLE_BUFFER_SIZE] = {0};

    for (;;)
    {
        if (xQueueReceive(txBLEQueue, &txBuffer, portMAX_DELAY) == pdTRUE)
        {
            if (!ble.isConnected() || ble.getTxCharacteristic() == nullptr)
            {
                continue;
            }
            
            ble.send(txBuffer);
            Serial.printf("[BLE] TX: %s\n", txBuffer);
        }
    }
}

void lifeBitTask(void *pvParameters)
{
    for (;;)
    {
        if (!ble.isConnected() || ble.getTxCharacteristic() == nullptr)
        {
            continue;
        }
            const char *msg = "alive\r\n";
            ble.getTxCharacteristic()->setValue((uint8_t *)msg, strlen(msg));
            ble.getTxCharacteristic()->notify();

        vTaskDelay(pdMS_TO_TICKS(BLE_LIFE_BIT_INTERVAL_MS));
    }
}