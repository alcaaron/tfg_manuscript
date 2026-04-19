#include "A3MESHPacket.h"
#include <cstring>


namespace lib::packet
{
    size_t A3MESHPacket::serializePacket(const A3MESHPacket_t &pkt, uint8_t *buffer)
    {
        size_t i = 0;

        buffer[i++] = pkt.pkt_id & 0xFF;
        buffer[i++] = pkt.pkt_id >> 8;

        buffer[i++] = pkt.node_id & 0xFF;
        buffer[i++] = pkt.node_id >> 8;

        buffer[i++] = pkt.src & 0xFF;
        buffer[i++] = pkt.src >> 8;

        buffer[i++] = pkt.dst & 0xFF;
        buffer[i++] = pkt.dst >> 8;

        buffer[i++] = pkt.ttl;
        buffer[i++] = pkt.type;
        buffer[i++] = pkt.flags;
        buffer[i++] = pkt.payload_len;

        memcpy(&buffer[i], pkt.payload, pkt.payload_len);
        i += pkt.payload_len;

        return i;
    }

    size_t A3MESHPacket::serializeLoRaMessage(const A3MESHHeader_t &header, const A3MESHPacket_t &pkt, uint8_t *buffer)
    {
        uint8_t temp[A3MESH_MAX_PACKET_SIZE];

        size_t packet_len = serializePacket(pkt, temp);
        
        size_t i = 0;

        buffer[i++] = header.type;
        buffer[i++] = header.version;
        buffer[i++] = header.packet_length & 0xFF;
        buffer[i++] = header.packet_length >> 8;

        memcpy(&buffer[i], temp, packet_len);
        i += packet_len;

        return i;
    }

    void A3MESHPacket::createLoRaMessage(uint8_t version, uint8_t type, uint16_t pkt_id, uint16_t node_id, uint16_t src, uint16_t dst, uint8_t ttl, uint8_t flags, const uint8_t *payload, uint8_t *buffer, size_t &len)
    {
        A3MESHHeader_t header = {
            .version = version,
            .type = type,
            .packet_length = 0 // Will be set after serialization
        };

        A3MESHPacket_t pkt = {
            .pkt_id = pkt_id,
            .node_id = node_id,
            .src = src,
            .dst = dst,
            .ttl = ttl,
            .type = type,
            .flags = flags,
            .payload_len = 0
        };

        if (payload)
        {
            size_t payload_len = strlen((const char *)payload);
            if (payload_len > A3MESH_MAX_PAYLOAD)
                payload_len = A3MESH_MAX_PAYLOAD;

            memcpy(pkt.payload, payload, payload_len);
            pkt.payload_len = payload_len;
        }

        header.packet_length = sizeof(pkt.pkt_id) + 
                                sizeof(pkt.node_id) + 
                                sizeof(pkt.src) + 
                                sizeof(pkt.dst) + 
                                sizeof(pkt.ttl) + 
                                sizeof(pkt.type) + 
                                sizeof(pkt.flags) + 
                                sizeof(pkt.payload_len) + 
                                pkt.payload_len;
        len = serializeLoRaMessage(header, pkt, buffer);
    }

    bool A3MESHPacket::deserializePacket(const uint8_t *buffer, size_t len, A3MESHHeader_t &header, A3MESHPacket_t &pkt)
    {
        if (len < sizeof(header))
            return false;

        header.type = buffer[0];
        header.version = buffer[1];
        header.packet_length = buffer[2] | (buffer[3] << 8);

        if (len < sizeof(header) + header.packet_length)
            return false;

        size_t i = sizeof(header);
        pkt.pkt_id = buffer[i] | (buffer[i + 1] << 8);
        i += 2;

        pkt.node_id = buffer[i] | (buffer[i + 1] << 8);
        i += 2;

        pkt.src = buffer[i] | (buffer[i + 1] << 8);
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