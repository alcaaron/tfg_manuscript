#pragma once
#include <stdint.h>
#include <cstddef>
#include <config.h>

namespace lib::packet
{
    /*!
     * \brief Identifies the purpose of an A3MESH frame at the header level.
     */
    typedef enum : uint8_t
    {
        k_data_plain      = 0, /*!< Unencrypted data payload. */
        k_discovery_type,      /*!< Neighbor discovery beacon. */
        k_key_exchange,        /*!< Key-exchange handshake payload. */
        k_data_encrypted,      /*!< Encrypted data payload (group or node-to-node). */
    } a3mesh_msg_type_t;

    /*!
     * \brief Identifies the encryption scheme applied to a data packet.
     *
     * Stored in A3MESH_packet_t::type for \c k_data_encrypted frames.
     */
    typedef enum : uint8_t
    {
        k_enc_none      = 0, /*!< No encryption (plain-text). */
        k_enc_group,         /*!< Group key encryption; group_id is prepended to payload. */
        k_enc_node2node,     /*!< Node-to-node (end-to-end) key encryption. */
    } a3mesh_enc_type_t;

    /*!< Fixed size of the A3MESH_header_t in bytes. */
    constexpr size_t HEADER_SIZE = 4;

    /*!< Sum of the sizes of all fixed fields in A3MESH_packet_t
     *   (pkt_id + src + node_id + dst + ttl + type + flags + payload_len + crc). */
    constexpr size_t FIXED_FIELDS_SIZE = 2 + 4 + 4 + 4 + 1 + 1 + 1 + 1 + 2;


    /*!
     * \brief A3MESH packet carrying one mesh protocol data unit.
     *
     * Serialised on the wire immediately after A3MESH_header_t.
     * The struct is packed to match the on-wire layout exactly.
     */
    typedef struct __attribute__((packed))
    {
        uint16_t pkt_id;                        /*!< Monotonically incrementing packet identifier, used for duplicate detection. */
        uint32_t src;                           /*!< Node ID of the original message source (never modified during forwarding). */
        uint32_t node_id;                       /*!< Node ID of the last forwarder; updated at each hop. */
        uint32_t dst;                           /*!< Destination node ID, or 0xFFFFFFFF for broadcast. */
        uint8_t  ttl;                           /*!< Time-to-live; decremented at each hop; dropped when it reaches 0. */
        uint8_t  type;                          /*!< Packet sub-type (e.g. a3mesh_enc_type_t for data frames). */
        uint8_t  flags;                         /*!< Bit-field of control flags (bit 0: direct/unicast hint). */
        uint8_t  payload_len;                   /*!< Number of valid bytes in payload[]. */
        uint16_t crc;                           /*!< CRC-16/CCITT of payload[0..payload_len-1]; computed on TX, verified on RX. */
        uint8_t  payload[A3MESH_MAX_PAYLOAD];   /*!< Variable-length payload; only the first payload_len bytes are valid. */
    } A3MESH_packet_t;


    /*!
     * \brief Fixed 4-byte frame header prepended to every A3MESH transmission.
     */
    typedef struct __attribute__((packed))
    {
        uint8_t           version;        /*!< Protocol version (current: A3M_VERSION). */
        a3mesh_msg_type_t type;           /*!< Frame type, determines how the payload is interpreted. */
        uint16_t          packet_length;  /*!< Length in bytes of the following A3MESH_packet_t. */
    } A3MESH_header_t;

    /*!
     * \brief Utility class with static methods for A3MESH packet serialisation
     *        and deserialisation.
     */
    class A3MESHPacket
    {
    public:
        /*!
         * \brief Computes the CRC-16/CCITT checksum over an arbitrary byte buffer.
         *
         * Uses polynomial 0x1021 with initial value 0xFFFF (no final XOR).
         * Applied over payload[0..payload_len-1] during serialisation and
         * re-applied during deserialisation for integrity verification.
         *
         * \param [IN] data  Pointer to the first byte to checksum.
         * \param [IN] len   Number of bytes to include in the computation.
         *
         * \retval uint16_t  Computed CRC-16/CCITT value.
         */
        static uint16_t compute_crc(const uint8_t *data, size_t len);

        /*!
         * \brief Serialises an A3MESH_packet_t into a raw byte buffer.
         *
         * Computes the CRC-16/CCITT of the payload and writes it into the
         * stream before the payload bytes.  Does not include the A3MESH_header_t;
         * call serialize_header_pkt() to produce a complete frame.
         *
         * \param [IN]  pkt     Packet to serialise.
         * \param [OUT] buffer  Destination buffer (must be at least
         *                      FIXED_FIELDS_SIZE + pkt.payload_len bytes).
         *
         * \retval size_t  Number of bytes written to buffer.
         */
        static size_t serialize_pkt(const A3MESH_packet_t &pkt, uint8_t *buffer);

        /*!
         * \brief Serialises a complete A3MESH frame (header + packet) into a
         *        raw byte buffer, ready to be handed to the LoRa driver.
         *
         * The header's packet_length field is filled automatically.
         *
         * \param [IN,OUT] header  Header to prepend; packet_length is updated.
         * \param [IN]     pkt     Packet to serialise after the header.
         * \param [OUT]    buffer  Destination buffer (must be at least
         *                         HEADER_SIZE + FIXED_FIELDS_SIZE + pkt.payload_len bytes).
         *
         * \retval size_t  Total number of bytes written (header + packet).
         */
        static size_t serialize_header_pkt(A3MESH_header_t &header, const A3MESH_packet_t &pkt, uint8_t *buffer);

        /*!
         * \brief Deserialises a raw byte buffer into an A3MESH header and packet.
         *
         * Performs version, length, payload sanity checks, and CRC-16/CCITT
         * integrity verification before accepting the frame.
         *
         * \param [IN]  buffer  Raw bytes received from the LoRa radio.
         * \param [IN]  len     Number of valid bytes in buffer.
         * \param [OUT] header  Populated header fields.
         * \param [OUT] pkt     Populated packet fields including payload copy.
         *
         * \retval  0   Success; frame is structurally valid and CRC matches.
         * \retval -1   Buffer too short to contain a header (len < HEADER_SIZE).
         * \retval -2   Buffer shorter than header.packet_length indicates.
         * \retval -3   payload_len exceeds A3MESH_MAX_PAYLOAD or overruns the buffer.
         * \retval -4   CRC mismatch; payload is corrupt.
         */
        static int8_t deserialize_pkt(const uint8_t *buffer, size_t len, A3MESH_header_t &header, A3MESH_packet_t &pkt);
    };

} // namespace lib::packet
