#include "A3MESHPacket.h"
#include <cstring>


namespace lib::packet
{
    size_t A3MESHPacket::serialize_pkt(const A3MESH_packet_t &pkt, uint8_t *buffer)
    {
        size_t i = 0;

        buffer[i++] = pkt.pkt_id & 0xFF;
        buffer[i++] = pkt.pkt_id >> 8;

        buffer[i++] = pkt.src & 0xFF;
        buffer[i++] = pkt.src >> 8;

        buffer[i++] = pkt.node_id & 0xFF;
        buffer[i++] = pkt.node_id >> 8;

        buffer[i++] = pkt.dst & 0xFF;
        buffer[i++] = pkt.dst >> 8;

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

        buffer[i++] = header.type;
        buffer[i++] = header.version;
        buffer[i++] = header.packet_length & 0xFF;
        buffer[i++] = header.packet_length >> 8;

        memcpy(&buffer[i], temp, packet_len);
        i += packet_len;

        return i;
    }

    bool A3MESHPacket::deserialize_pkt(const uint8_t *buffer, size_t len, A3MESH_header_t &header, A3MESH_packet_t &pkt)
    {
        if (len < sizeof(header))
            return false;

        header.version = buffer[0];
        header.type = (a3mesh_msg_type_t)buffer[1];
        header.packet_length = buffer[2] | (buffer[3] << 8);

        if (len < sizeof(header) + header.packet_length)
            return false;

        size_t i = sizeof(header);
        pkt.pkt_id = buffer[i] | (buffer[i + 1] << 8);
        i += 2;

        pkt.src = buffer[i] | (buffer[i + 1] << 8);
        i += 2;

        pkt.node_id = buffer[i] | (buffer[i + 1] << 8);
        i += 2;

        pkt.dst = buffer[i] | (buffer[i + 1] << 8);
        i += 2;

        pkt.ttl = buffer[i++];
        pkt.type = buffer[i++];
        pkt.flags = buffer[i++];
        pkt.payload_len = buffer[i++];

        if (pkt.payload_len > A3MESH_MAX_PAYLOAD || i + pkt.payload_len > len)
            return false;

        memcpy(pkt.payload, &buffer[i], pkt.payload_len);
        return true;
    }
} // namespace lib::packet