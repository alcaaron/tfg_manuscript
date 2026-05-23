#include "BLETasks.h"

extern lib::ble::BLEDriver ble;

void vBLETxTask(void *pvParameters)
{
    char tx_buffer[BLE_BUFFER_SIZE] = {0};

    for (;;)
    {
        if (xQueueReceive(xBLETxQueue, &tx_buffer, portMAX_DELAY) == pdTRUE)
        {
            ble.send(tx_buffer);
            Serial.printf("[BLE] TX: %s", tx_buffer);
        }
    }
}

void vBLELifeBitTask(void *pvParameters)
{
    for (;;)
    {
        ble.send("alive\r\n");
        vTaskDelay(pdMS_TO_TICKS(BLE_LIFE_BIT_INTERVAL_MS));
    }
}