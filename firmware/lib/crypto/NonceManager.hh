#pragma once
#include "CryptoTypes.hh"
#include <atomic>
#include <stdint.h>

namespace lib::crypto {

/*!
 * \brief Generates and reconstructs 96-bit AES-256-GCM nonces.
 *
 * Nonce layout (12 bytes, little-endian):
 *   [sender_node_id 4B] [counter 8B]
 *
 * The node_id prefix ensures that different senders using the same shared
 * key (network / group) never produce the same nonce.  The 64-bit counter
 * is transmitted in the LoRa payload alongside the ciphertext so the
 * receiver can reconstruct the full nonce without any explicit nonce field
 * in the frame header.
 *
 * A single atomic counter is shared across all key domains because each
 * (node_id, counter) pair is unique by construction.
 */
class NonceManager {
public:
    NonceManager();

    /*!
     * \brief Build the next TX nonce and advance the counter atomically.
     *
     * \param node_id   This node's identifier (first 4 bytes of nonce).
     * \param out_nonce Output: 12-byte nonce ready for AES-256-GCM.
     */
    void next_nonce(uint32_t node_id, uint8_t out_nonce[CRYPTO_NONCE_SIZE]);

    /*!
     * \brief Reconstruct a nonce from a received counter value and sender ID.
     *
     * Call on RX after reading the 8-byte counter from the payload.
     *
     * \param sender_id  pkt.src of the received frame.
     * \param ctr        8-byte little-endian counter from the payload.
     * \param out_nonce  Output: 12-byte nonce.
     */
    static void build_nonce(uint32_t sender_id,
                            const uint8_t ctr[CRYPTO_NONCE_CTR_SIZE],
                            uint8_t out_nonce[CRYPTO_NONCE_SIZE]);

private:
    std::atomic<uint64_t> _counter{1}; // 0 is reserved; start at 1
};

} // namespace lib::crypto
