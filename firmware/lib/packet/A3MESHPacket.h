#pragma once
#include <stdint.h>
#include <cstddef>

#define A3MESH_MAX_PAYLOAD 220

namespace lib::packet
{
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

    struct A3MESHHeader {
        uint8_t  version;
        uint8_t  type;
        uint16_t length;
    };

} // namespace lib::packet