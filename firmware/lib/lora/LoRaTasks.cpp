#include "LoRaTasks.h"

extern lib::lora::LoRaDriver lora;

void rxLoRaPacketTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = POLLING_INTERVAL_MS / portTICK_PERIOD_MS;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        int packetSize = LoRa.parsePacket();

        if (packetSize > sizeof(lib::packet::A3MESHHeader_t))
        {
            lib::lora::RxMessage_t msg;
            msg.len = LoRa.readBytes(msg.rx_data, sizeof(msg.rx_data));

            Serial.printf("[LORA] packet received, length: %d\r\n", msg.len);

            if (msg.len <= 0 || msg.len > sizeof(msg.rx_data)) { continue; }
            
            if (msg.len > sizeof(lib::packet::A3MESHHeader_t))
            {
                xQueueSend(rxLoRaQueue, &msg, portMAX_DELAY);
            }
        }
    }
}

void txLoRaPacketTask(void *pvParameters)
{
    lib::lora::TxMessage_t msg;

    for (;;)
    {
        if (xQueueReceive(txLoRaQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            lora.sendRawPacket(msg.tx_data, msg.len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}