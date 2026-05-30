#include "LoRaTasks.h"

extern lib::lora::LoRaDriver lora;

void vLoRaRxTask(void *pvParameters)
{
    for (;;)
    {
        if (!lib::lora::LoRaDriver::consume_rx_flag())
        {
            vTaskDelay(1);
            continue;
        }

        int pkt_size = LoRa.parsePacket();
        if (pkt_size <= 0)
            continue;

        if (pkt_size > LORA_MAX_PACKET_SIZE)
        {
            while (LoRa.available()) LoRa.read();
            continue;
        }

        // -------------------------
        // ZERO-COPY FROM DRIVER POOL
        // -------------------------
        auto *frame = lib::lora::LoRaDriver::alloc_frame();

        frame->len = pkt_size;
        frame->rssi = LoRa.packetRssi();

        LoRa.readBytes(frame->rx_data, pkt_size);
        LoRa.receive();

        // -------------------------
        // SEND POINTER (NO COPY)
        // -------------------------
        xQueueSend(xLoRaRxQueue, &frame, 0);
    }
}

void vLoRaTxTask(void *pvParameters)
{
    lib::lora::LoRaDriver::tx_LoRa_msg_t msg;

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