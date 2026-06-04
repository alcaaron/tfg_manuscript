#include "NonceManager.hh"
#include <cstring>

namespace lib::crypto {

NonceManager::NonceManager() {}

void NonceManager::next_nonce(uint32_t node_id, uint8_t out_nonce[CRYPTO_NONCE_SIZE])
{
    uint64_t ctr = _counter.fetch_add(1, std::memory_order_relaxed);

    out_nonce[0] = (node_id >>  0) & 0xFF;
    out_nonce[1] = (node_id >>  8) & 0xFF;
    out_nonce[2] = (node_id >> 16) & 0xFF;
    out_nonce[3] = (node_id >> 24) & 0xFF;

    for (int i = 0; i < 8; i++)
        out_nonce[4 + i] = (ctr >> (8 * i)) & 0xFF;
}

void NonceManager::build_nonce(uint32_t sender_id,
                               const uint8_t ctr[CRYPTO_NONCE_CTR_SIZE],
                               uint8_t out_nonce[CRYPTO_NONCE_SIZE])
{
    out_nonce[0] = (sender_id >>  0) & 0xFF;
    out_nonce[1] = (sender_id >>  8) & 0xFF;
    out_nonce[2] = (sender_id >> 16) & 0xFF;
    out_nonce[3] = (sender_id >> 24) & 0xFF;
    memcpy(out_nonce + 4, ctr, CRYPTO_NONCE_CTR_SIZE);
}

} // namespace lib::crypto
