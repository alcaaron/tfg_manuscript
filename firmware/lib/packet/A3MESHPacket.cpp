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

    uint16_t A3MESHPacket::compute_crc(const uint8_t *data, size_t len)
    {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < len; i++) {
            crc ^= (uint16_t)data[i] << 8;
            for (int j = 0; j < 8; j++) {
                crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
            }
        }
        return crc;
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
        buffer[i++] = pkt.type;
        buffer[i++] = pkt.flags;
        buffer[i++] = pkt.payload_len;

        uint16_t crc = compute_crc(pkt.payload, pkt.payload_len);
        buffer[i++] = crc & 0xFF;
        buffer[i++] = crc >> 8;

        memcpy(&buffer[i], pkt.payload, pkt.payload_len);
        i += pkt.payload_len;

        return i;
    }

    size_t A3MESHPacket::serialize_header_pkt(A3MESH_header_t &header, const A3MESH_packet_t &pkt, uint8_t *buffer)
    {
        size_t packet_len = serialize_pkt(pkt, buffer + HEADER_SIZE);

        header.packet_length = packet_len;

        // Version string: copied up to its terminator and zero-padded to
        // VERSION_SIZE, so the field is always NUL-terminated on the wire.
        const size_t ver_len = strnlen(header.version, VERSION_SIZE - 1);
        memcpy(buffer, header.version, ver_len);
        memset(buffer + ver_len, 0, VERSION_SIZE - ver_len);

        size_t i = VERSION_SIZE;
        buffer[i++] = header.type;
        buffer[i++] = packet_len & 0xFF;
        buffer[i++] = packet_len >> 8;

        return HEADER_SIZE + packet_len;
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
        // HEADER (fixed HEADER_SIZE bytes)
        // -----------------------------
        memcpy(header.version, &buffer[i], VERSION_SIZE);
        header.version[VERSION_SIZE - 1] = '\0';   // never trust the sender's terminator
        i += VERSION_SIZE;

        header.type = (a3mesh_msg_type_t)buffer[i++];
        header.packet_length = read_u16(buffer, i);

        // -----------------------------
        // Length validation
        // -----------------------------
        // The packet must at least hold its fixed fields and must not
        // extend past the received bytes.
        if (header.packet_length < FIXED_FIELDS_SIZE ||
            len < HEADER_SIZE + header.packet_length)
            return -2;

        // -----------------------------
        // FIXED FIELDS (little endian, byte-wise: buffer may be unaligned)
        // -----------------------------
        pkt.pkt_id  = read_u16(buffer, i);
        pkt.src     = read_u32(buffer, i);
        pkt.node_id = read_u32(buffer, i);
        pkt.dst     = read_u32(buffer, i);

        pkt.ttl         = buffer[i++];
        pkt.type        = buffer[i++];
        pkt.flags       = buffer[i++];
        pkt.payload_len = buffer[i++];

        const uint16_t stored_crc = read_u16(buffer, i);

        // -----------------------------
        // Payload validation
        // -----------------------------
        if (pkt.payload_len > A3MESH_MAX_PAYLOAD ||
            FIXED_FIELDS_SIZE + pkt.payload_len > header.packet_length)
        {
            return -3;
        }

        // -----------------------------
        // Payload copy
        // -----------------------------
        memcpy(pkt.payload, &buffer[i], pkt.payload_len);

        // -----------------------------
        // CRC verification
        // -----------------------------
        if (compute_crc(pkt.payload, pkt.payload_len) != stored_crc)
            return -4;

        pkt.crc = stored_crc;

        return 0;
    }
} // namespace lib::packet
