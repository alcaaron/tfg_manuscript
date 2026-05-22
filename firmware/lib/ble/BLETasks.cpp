#include "BLETasks.h"

extern lib::ble::BLEDriver ble;

void BLEtxTask(void *pvParameters)
{
    char txBuffer[BLE_BUFFER_SIZE] = {0};

    for (;;)
    {
        if (xQueueReceive(txBLEQueue, &txBuffer, portMAX_DELAY) == pdTRUE)
        {
            ble.send(txBuffer);
            Serial.printf("[BLE] TX: %s", txBuffer);
        }
    }
}

void lifeBitTask(void *pvParameters)
{
    for (;;)
    {
        ble.send("alive\r\n");
        vTaskDelay(pdMS_TO_TICKS(BLE_LIFE_BIT_INTERVAL_MS));
    }
}