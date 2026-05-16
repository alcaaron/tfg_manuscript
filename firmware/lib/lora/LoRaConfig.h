#pragma once
#include <stdint.h>
#include <config.h>

namespace lib::lora
{
    struct A3MESHConfig
    {
        // LoRa pins
        uint8_t nss_pin = NSS_GPIO;
        uint8_t rst_pin = RST_GPIO;
        uint8_t dio0_pin = DIO0_GPIO;

        long frequency = EU868_FREQUENCY;
        float bandwidth = BANDWIDTH_125_KHZ;
        uint8_t sf = A3MESH_SF; // LoRa spreading factor. Allowed values range from 6 to 12.
        uint8_t cr = A3MESH_CR; // LoRa coding rate denominator. Allowed values range from 5 to 8.
        uint8_t sync_word = A3MESH_SYNC_WORD; // LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
        int8_t power = A3MESH_POWER; // Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
        uint16_t preamble_len = A3MESH_PREAMBLE_LENGTH; // Length of LoRa transmission preamble in symbols. The actual preamble length is 4.25 symbols longer than the set number. Allowed values range from 6 to 65535.
    };

} // namespace lib::lora