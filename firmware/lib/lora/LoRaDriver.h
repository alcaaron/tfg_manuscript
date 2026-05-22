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
    typedef struct
    {
        uint8_t rx_data[LORA_MAX_PACKET_SIZE];
        size_t len;
        int16_t rssi;
    } RxMessage_t;

    typedef struct
    {
        uint8_t tx_data[LORA_MAX_PACKET_SIZE];
        size_t len;
    } TxMessage_t;

    class LoRaDriver
    {
    public:
        LoRaDriver();

        bool begin(const lora::A3MESHConfig &config);
        bool sendRawPacket(const uint8_t &data, size_t len);
    };
} // namespace lib::lora