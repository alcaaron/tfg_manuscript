#pragma once
#include <stdint.h>
#include <cstddef>
#include <config.h>

namespace lib::packet
{
    typedef enum : uint8_t
    {
        k_data_type = 0,
        k_discovery_type
    } a3mesh_msg_type_t;

    typedef struct
    {
        uint16_t pkt_id;                        // 2 bytes
        uint16_t src;                           // 2 bytes
        uint16_t node_id;                       // 2 bytes
        uint16_t dst;                           // 2 bytes
        uint8_t ttl;                            // 1 byte
        uint8_t type;                           // 1 byte provision field

        uint8_t flags;                          // 1 byte
        uint8_t payload_len;                    // 1 byte
        uint8_t payload[A3MESH_MAX_PAYLOAD];    // (256 - N=15) bytes, N representing the sum of the header size and the fixed fields in the packet structure
    } A3MESH_packet_t;


    typedef struct
    {
        uint8_t version;                        // 1 byte
        a3mesh_msg_type_t type;                 // 1 byte
        uint16_t packet_length;                 // 2 bytes
    } A3MESH_header_t;

    class A3MESHPacket
    {
    public:
        static size_t serialize_pkt(const A3MESH_packet_t &pkt, uint8_t *buffer);
        static size_t serialize_header_pkt(const A3MESH_header_t &header, const A3MESH_packet_t &pkt, uint8_t *buffer);
        static bool deserialize_pkt(const uint8_t *buffer, size_t len, A3MESH_header_t &header, A3MESH_packet_t &pkt);
    };

} // namespace lib::packet