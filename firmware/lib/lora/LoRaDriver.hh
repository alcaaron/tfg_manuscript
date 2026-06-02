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
            uint16_t pkt_id;      // mesh packet id, used for passive-ACK check
            uint32_t src;         // mesh source node id, used for passive-ACK check
            bool     is_forward;  // true = subject to passive-ACK cancellation
        } tx_LoRa_msg_t;

        bool begin(const lora::A3MESHConfig &config);
        bool send_raw_pkt(const uint8_t &data, size_t len);

        static void IRAM_ATTR onLoRaRxIRQ();
        static bool consume_rx_flag();
        static rx_LoRa_msg_t* alloc_frame();

        // Returns estimated time-on-air in ms for a packet of pkt_len bytes,
        // using the configured SF/BW/CR/preamble from config.h.
        static uint32_t compute_airtime_ms(size_t pkt_len);

    private:
        static volatile bool lora_rx_flag;

        static rx_LoRa_msg_t pool[8];
        static volatile uint8_t pool_idx;

        // Duty-cycle tracking: ring buffer of recent TX records (tx timestamp + duration).
        // Accessed only from vLoRaTxTask — no mutex needed.
        struct AirtimeEntry {
            uint32_t ts_ms;
            uint32_t dur_ms;
        };
        AirtimeEntry _airtime_log[LORA_AIRTIME_RECORDS] = {};
        uint8_t      _airtime_log_idx = 0;

        // Returns true and records airtime if TX is within duty-cycle budget; false otherwise.
        bool check_and_record_airtime(uint32_t airtime_ms);
    };
} // namespace lib::lora