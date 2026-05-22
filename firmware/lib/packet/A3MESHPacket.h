#pragma once
#include <stdint.h>
#include <cstddef>
#include <config.h>

namespace lib::packet
{
    typedef enum : uint8_t
    {
        MSG_TYPE_DATA = 0,
        MSG_TYPE_DISCOVERY
    } MessageType;

    typedef struct
    {
        uint16_t pkt_id;                        // 2 bytes
        uint16_t node_id;                       // 2 bytes
        uint16_t src;                           // 2 bytes
        uint16_t dst;                           // 2 bytes
        uint16_t neighbor_id;                   // 2 bytes
        uint8_t ttl;                            // 1 byte
        uint8_t flags;                          // 1 byte
        uint8_t payload_len;                    // 1 byte
        uint8_t payload[A3MESH_MAX_PAYLOAD];    // (256 - N=17) bytes, N representing the sum of the header size and the fixed fields in the packet structure
    } A3MESHPacket_t;

    typedef struct
    {
        uint8_t version;                        // 1 byte
        MessageType type;                       // 1 byte
        uint16_t packet_length;                 // 2 bytes
    } A3MESHHeader_t;

    class A3MESHPacket
    {
    public:
        static size_t serializePacket(const A3MESHPacket_t &pkt, uint8_t *buffer);
        static size_t serializeLoRaMessage(const A3MESHHeader_t &header, const A3MESHPacket_t &pkt, uint8_t *buffer);
        static bool deserializePacket(const uint8_t *buffer, size_t len, A3MESHHeader_t &header, A3MESHPacket_t &pkt);
        static void createLoRaMessage(uint8_t version, MessageType type, uint16_t pkt_id, uint16_t node_id, uint16_t src, uint16_t dst, uint16_t neighbor_id, uint8_t ttl, uint8_t flags, const char *payload, uint8_t *buffer, size_t &len);
    };

} // namespace lib::packet