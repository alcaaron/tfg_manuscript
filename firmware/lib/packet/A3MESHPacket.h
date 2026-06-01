#pragma once
#include <stdint.h>
#include <cstddef>
#include <config.h>

namespace lib::packet
{
    typedef enum : uint8_t
    {
        k_data_plain = 0,
        k_discovery_type,
        k_key_exchange,
        k_data_encrypted,
    } a3mesh_msg_type_t;

    typedef enum : uint8_t
    {
        k_enc_none = 0,
        k_enc_group,
        k_enc_node2node,
    } a3mesh_enc_type_t;

    constexpr size_t HEADER_SIZE = 4;
    // Sum of sizes of pkt_id, src, node_id, dst, ttl, type, flags, payload_len
    constexpr size_t FIXED_FIELDS_SIZE = 2 + 4 + 4 + 4 + 1 + 1 + 1 + 1; 
    

    typedef struct __attribute__((packed))
    {
        uint16_t pkt_id;                        // 2 bytes
        uint32_t src;                           // 4 bytes
        uint32_t node_id;                       // 4 bytes
        uint32_t dst;                           // 4 bytes
        uint8_t ttl;                            // 1 byte
        uint8_t type;                           // 1 byte provision field

        uint8_t flags;                          // 1 byte
        uint8_t payload_len;                    // 1 byte
        uint8_t payload[A3MESH_MAX_PAYLOAD];    // (256 - N=21) bytes, N representing the sum of the header size and the fixed fields in the packet structure
    } A3MESH_packet_t;


    typedef struct __attribute__((packed))
    {
        uint8_t version;                        // 1 byte
        a3mesh_msg_type_t type;                 // 1 byte
        uint16_t packet_length;                 // 2 bytes
    } A3MESH_header_t;

    class A3MESHPacket
    {
    public:
        static size_t serialize_pkt(const A3MESH_packet_t &pkt, uint8_t *buffer);
        static size_t serialize_header_pkt(A3MESH_header_t &header, const A3MESH_packet_t &pkt, uint8_t *buffer);
        static int8_t deserialize_pkt(const uint8_t *buffer, size_t len, A3MESH_header_t &header, A3MESH_packet_t &pkt);
    };

} // namespace lib::packet