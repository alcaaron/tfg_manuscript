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