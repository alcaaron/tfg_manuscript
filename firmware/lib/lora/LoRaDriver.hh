#pragma once

#include <LoRa.h>
#include <SPI.h>
#include "LoRaConfig.h"
#include <config.h>
#include "A3MESHPacket.h"
#include <freertos/task.h>
#include <freertos/queue.h>

namespace lib::lora
{
    class LoRaDriver
    {
    public:
        LoRaDriver();

        typedef struct
        {
            uint8_t rx_data[LORA_MAX_PACKET_SIZE];
            size_t len;
            int16_t rssi;
        } rx_LoRa_msg_t;

        typedef struct
        {
            uint8_t  tx_data[LORA_MAX_PACKET_SIZE];
            size_t   len;
            uint16_t delay_ms;
        } tx_LoRa_msg_t;

        bool begin(const lora::A3MESHConfig &config);
        bool send_raw_pkt(const uint8_t &data, size_t len);

        static void IRAM_ATTR onLoRaRxIRQ();
        static bool consume_rx_flag();
        static rx_LoRa_msg_t* alloc_frame();

    private:
        static volatile bool lora_rx_flag;

        static rx_LoRa_msg_t pool[8];
        static volatile uint8_t pool_idx;
    };
} // namespace lib::lora