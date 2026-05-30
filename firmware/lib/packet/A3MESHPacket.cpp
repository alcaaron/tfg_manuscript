#include "A3MESHPacket.h"
#include <cstring>


namespace lib::packet
{
    static inline uint16_t read_u16(const uint8_t* b, size_t& i)
    {
        uint16_t v = (uint16_t)b[i] | ((uint16_t)b[i + 1] << 8);
        i += 2;
        return v;
    }

    static inline uint32_t read_u32(const uint8_t* b, size_t& i)
    {
        uint32_t v =
            ((uint32_t)b[i]) |
            ((uint32_t)b[i + 1] << 8) |
            ((uint32_t)b[i + 2] << 16) |
            ((uint32_t)b[i + 3] << 24);
        i += 4;
        return v;
    }

    size_t A3MESHPacket::serialize_pkt(const A3MESH_packet_t &pkt, uint8_t *buffer)
    {
        size_t i = 0;

        buffer[i++] = pkt.pkt_id & 0xFF;
        buffer[i++] = pkt.pkt_id >> 8;

        buffer[i++] = (pkt.src >> 0)  & 0xFF;
        buffer[i++] = (pkt.src >> 8)  & 0xFF;
        buffer[i++] = (pkt.src >> 16) & 0xFF;
        buffer[i++] = (pkt.src >> 24) & 0xFF;

        buffer[i++] = (pkt.node_id >> 0)  & 0xFF;
        buffer[i++] = (pkt.node_id >> 8)  & 0xFF;
        buffer[i++] = (pkt.node_id >> 16) & 0xFF;
        buffer[i++] = (pkt.node_id >> 24) & 0xFF;

        buffer[i++] = (pkt.dst >> 0)  & 0xFF;
        buffer[i++] = (pkt.dst >> 8)  & 0xFF;
        buffer[i++] = (pkt.dst >> 16) & 0xFF;
        buffer[i++] = (pkt.dst >> 24) & 0xFF;

        buffer[i++] = pkt.ttl;
        buffer[i++] = pkt.type; // provision field
        buffer[i++] = pkt.flags;
        buffer[i++] = pkt.payload_len;

        memcpy(&buffer[i], pkt.payload, pkt.payload_len);
        i += pkt.payload_len;

        return i;
    }

    size_t A3MESHPacket::serialize_header_pkt(A3MESH_header_t &header, const A3MESH_packet_t &pkt, uint8_t *buffer)
    {
        uint8_t temp[LORA_MAX_PACKET_SIZE];

        size_t packet_len = serialize_pkt(pkt, temp);

        header.packet_length = packet_len;
        
        size_t i = 0;

        buffer[i++] = header.version;
        buffer[i++] = header.type;
        buffer[i++] = header.packet_length & 0xFF;
        buffer[i++] = header.packet_length >> 8;

        memcpy(&buffer[i], temp, packet_len);
        i += packet_len;

        return i;
    }

    int8_t A3MESHPacket::deserialize_pkt(const uint8_t *buffer, size_t len, A3MESH_header_t &header, A3MESH_packet_t &pkt)
    {
        // -----------------------------
        // Basic sanity check
        // -----------------------------
        if (len < HEADER_SIZE)
            return -1;

        size_t i = 0;

        // -----------------------------
        // HEADER (fixed 4 bytes)
        // -----------------------------
        header.version = buffer[i++];
        header.type = (a3mesh_msg_type_t)buffer[i++];

        header.packet_length =
            (uint16_t)buffer[i] |
            ((uint16_t)buffer[i + 1] << 8);

        i += 2;

        // -----------------------------
        // Length validation
        // -----------------------------
        const size_t expected_len = HEADER_SIZE + header.packet_length;

        if (len < expected_len)
            return -2;

        // -----------------------------
        // PAYLOAD POINTER BASE
        // -----------------------------
        const uint8_t* p = buffer + HEADER_SIZE;

        // -----------------------------
        // FIXED FIELDS
        // -----------------------------
        pkt.pkt_id =
            *(const uint16_t*)p;
        p += 2;

        pkt.src =
            *(const uint32_t*)p;
        p += 4;

        pkt.node_id =
            *(const uint32_t*)p;
        p += 4;

        pkt.dst =
            *(const uint32_t*)p;
        p += 4;

        pkt.ttl = *p++;
        pkt.type = *p++;
        pkt.flags = *p++;
        pkt.payload_len = *p++;

        // -----------------------------
        // Payload validation
        // -----------------------------
        if (pkt.payload_len > A3MESH_MAX_PAYLOAD ||
            (size_t)(p - buffer) + pkt.payload_len > len)
        {
            return -3;
        }

        // -----------------------------
        // Payload copy
        // -----------------------------
        memcpy(pkt.payload, p, pkt.payload_len);

        return 0;
    }
} // namespace lib::packet