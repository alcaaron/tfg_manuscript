#pragma once
#include <stdint.h>
#include <cstddef>
#include <config.h>

namespace lib::packet
{
    typedef enum
    {
        MSG_TYPE_DATA = 0,
        MSG_TYPE_DISCOVERY
    } MessageType;

    typedef struct
    {
        uint16_t pkt_id;
        uint16_t node_id;
        uint16_t src;
        uint16_t dst;

        uint8_t ttl;
        uint8_t type;

        uint8_t flags;

        uint8_t payload_len;
        uint8_t payload[A3MESH_MAX_PAYLOAD];
    } A3MESHPacket_t;

    typedef struct
    {
        uint8_t version;
        uint8_t type;
        uint16_t packet_length;
    } A3MESHHeader_t;

    enum class Transport : uint8_t
    {
        BLE,
        LORA
    };

    class A3MESHPacket
    {
    public:
        static size_t serializePacket(const A3MESHPacket_t &pkt, uint8_t *buffer);
        static size_t serializeLoRaMessage(const A3MESHHeader_t &header, const A3MESHPacket_t &pkt, uint8_t *buffer);
        static bool deserializePacket(const uint8_t *buffer, size_t len, A3MESHHeader_t &header, A3MESHPacket_t &pkt);
        //static bool deserializeLoRaMessage(const uint8_t *buffer, size_t len, A3MESHHeader_t &header, A3MESHPacket_t &pkt);
        static void createLoRaMessage(uint8_t version, uint8_t type, uint16_t pkt_id, uint16_t node_id, uint16_t src, uint16_t dst, uint8_t ttl, uint8_t flags, const uint8_t *payload, uint8_t *buffer, size_t &len);

    };

} // namespace lib::packet