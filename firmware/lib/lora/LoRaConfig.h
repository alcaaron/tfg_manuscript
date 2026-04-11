#pragma once
#include <stdint.h>
#include <config.h>

namespace lib::lora
{
    struct A3MESHConfig
    {
        // LoRa pins
        uint8_t cs_pin = CS_PIN;
        uint8_t irq_pin = IRQ_PIN;
        uint8_t rst_pin = RST_PIN;
        uint8_t dio1_pin = DIO1_PIN;

        float frequency = EU868_FREQUENCY;
        float bandwidth = BANDWIDTH_125_KHZ;
        uint8_t sf = A3MESH_SF; // LoRa spreading factor. Allowed values range from 6 to 12.
        uint8_t cr = A3MESH_CR; // LoRa coding rate denominator. Allowed values range from 5 to 8.
        uint8_t sync_word = A3MESH_SYNC_WORD; // LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
        int8_t power = A3MESH_POWER; // Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
        uint16_t preamble_len = A3MESH_PREAMBLE_LENGTH; // Length of LoRa transmission preamble in symbols. The actual preamble length is 4.25 symbols longer than the set number. Allowed values range from 6 to 65535.
        // MAX packet size per packet in bytes. It could be changed between 13 and 255 bytes. Recommended 100 or less bytes.
        // If exceed it will be automatically separated through multiple packets 
        // In bytes (226 bytes [UE max allowed with SF7 and 125khz])
        // MAX payload size for hello packets = A3MESH_MAX_PACKET_SIZE - 7 bytes of header
        // MAX payload size for data packets = A3MESH_MAX_PACKET_SIZE - 7 bytes of header - 2 bytes of via
        // MAX payload size for reliable and large packets = A3MESH_MAX_PACKET_SIZE - 7 bytes of header - 2 bytes of via - 3 of control packet.
        // Having different max_packet_size in the same network will cause problems.
        //size_t max_packet_size = A3MESH_MAX_PACKET_SIZE;
    };

} // namespace lib::lora