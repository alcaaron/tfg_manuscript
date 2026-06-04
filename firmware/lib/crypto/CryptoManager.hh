#pragma once
#include "CryptoTypes.hh"
#include "CryptoKeys.hh"
#include "NonceManager.hh"
#include "ReplayProtection.hh"
#include "A3MESHPacket.h"

namespace lib::crypto {

/*!
 * \brief Top-level cryptographic layer for A3MESH.
 *
 * Encrypts outgoing payloads and decrypts incoming ones for three security
 * domains (node-to-node, group, network).  On success the methods update
 * pkt.payload and pkt.payload_len directly so the rest of the TX/RX
 * pipeline does not need to know about the crypto internals.
 *
 * Wire payload layout:
 *
 *   Node-to-node / Network:
 *     [nonce_ctr 8B LE] [tag 16B] [ciphertext ...]
 *
 *   Group:
 *     [group_id 4B LE] [nonce_ctr 8B LE] [tag 16B] [ciphertext ...]
 *
 * AAD (binds ciphertext to routing fields, prevents cross-frame reuse):
 *   node-to-node / network : pkt_id (2B) + src (4B) + dst (4B) = 10 B
 *   group                  : pkt_id (2B) + src (4B) + group_id (4B) = 10 B
 */
class CryptoManager {
public:
    CryptoManager() = default;

    CryptoKeys       keys;
    NonceManager     nonce;
    ReplayProtection replay;

    // ── TX ──────────────────────────────────────────────────────────────────

    /*!
     * \brief Encrypt plaintext for a specific peer (node-to-node key).
     *
     * Writes the encrypted payload into pkt.payload and sets pkt.payload_len.
     * Requires a peer key for \p dst to be provisioned via keys.set_peer_key().
     */
    CryptoError encrypt_n2n(uint32_t my_id, uint32_t dst,
                            const uint8_t *plaintext, size_t pt_len,
                            lib::packet::A3MESH_packet_t &pkt);

    /*!
     * \brief Encrypt with the shared network key (broadcasts or semi-public).
     *
     * Requires a network key to be provisioned via keys.set_net_key().
     */
    CryptoError encrypt_net(uint32_t my_id,
                            const uint8_t *plaintext, size_t pt_len,
                            lib::packet::A3MESH_packet_t &pkt);

    /*!
     * \brief Encrypt for a group channel.
     *
     * Prepends group_id (4 B LE) then the nonce+tag+ciphertext block.
     * Requires a group key provisioned via keys.set_group_key().
     */
    CryptoError encrypt_grp(uint32_t my_id, uint32_t group_id,
                            const uint8_t *plaintext, size_t pt_len,
                            lib::packet::A3MESH_packet_t &pkt);

    // ── RX ──────────────────────────────────────────────────────────────────

    /*!
     * \brief Decrypt a node-to-node packet.
     *
     * On success pkt.payload and pkt.payload_len are overwritten with the
     * plaintext.  \p out_buf (size \p out_cap) is used as a scratch buffer
     * and must hold at least pkt.payload_len - CRYPTO_N2N_OVERHEAD bytes.
     */
    CryptoError decrypt_n2n(uint32_t sender_id,
                            lib::packet::A3MESH_packet_t &pkt,
                            uint8_t *out_buf, size_t out_cap);

    /*!
     * \brief Decrypt a network-encrypted packet.
     */
    CryptoError decrypt_net(uint32_t sender_id,
                            lib::packet::A3MESH_packet_t &pkt,
                            uint8_t *out_buf, size_t out_cap);

    /*!
     * \brief Decrypt a group packet.
     *
     * \param out_group_id  Receives the group_id parsed from the payload header.
     */
    CryptoError decrypt_grp(uint32_t sender_id,
                            lib::packet::A3MESH_packet_t &pkt,
                            uint8_t *out_buf, size_t out_cap,
                            uint32_t &out_group_id);

private:
    CryptoError _encrypt_with_key(uint32_t my_id,
                                  const uint8_t key[CRYPTO_KEY_SIZE],
                                  const uint8_t *plaintext, size_t pt_len,
                                  const uint8_t *aad, size_t aad_len,
                                  uint8_t *out_payload, uint8_t &out_len);

    CryptoError _decrypt_with_key(uint32_t sender_id,
                                  const uint8_t key[CRYPTO_KEY_SIZE],
                                  const uint8_t *payload, size_t payload_len,
                                  const uint8_t *aad, size_t aad_len,
                                  uint8_t *out, size_t out_cap,
                                  size_t &out_len);
};

} // namespace lib::crypto
