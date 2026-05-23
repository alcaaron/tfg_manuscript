#include "LoRaTasks.h"

extern lib::lora::LoRaDriver lora;

void vLoRaRxTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = POLLING_INTERVAL_MS / portTICK_PERIOD_MS;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        int packetSize = LoRa.parsePacket();

        if (packetSize > sizeof(lib::packet::A3MESH_header_t))
        {
            lib::lora::rx_LoRa_msg_t msg;
            msg.len = LoRa.readBytes(msg.rx_data, sizeof(msg.rx_data));
            msg.rssi = LoRa.packetRssi();

            //Serial.printf("[LORA] packet received, length: %d, rssi: %d\r\n", msg.len, msg.rssi);

            if (msg.len <= 0 || msg.len > sizeof(msg.rx_data)) { continue; }
            
            if (msg.len > sizeof(lib::packet::A3MESH_header_t))
            {
                xQueueSend(xLoRaRxQueue, &msg, portMAX_DELAY);
            }
        }
    }
}

void vLoRaTxTask(void *pvParameters)
{
    lib::lora::tx_LoRa_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(xLoRaTxQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            //Serial.printf("[LORA] Sending packet, length: %d\r\n", msg.len);
            lora.send_raw_pkt(*(msg.tx_data), msg.len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}