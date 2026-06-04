#pragma once
#include <stdint.h>
#include <stddef.h>

namespace lib::crypto {

constexpr size_t CRYPTO_KEY_SIZE       = 32;   // AES-256-GCM: 256-bit key
constexpr size_t CRYPTO_NONCE_SIZE     = 12;   // RFC 7539: 96-bit nonce
constexpr size_t CRYPTO_NONCE_CTR_SIZE = 8;    // 64-bit counter transmitted in payload
constexpr size_t CRYPTO_TAG_SIZE       = 16;   // Poly1305 tag: 128 bits

// Wire overhead added to the LoRa payload by encryption:
//   node2node / network : [nonce_ctr 8B] + [tag 16B]       = 24 B
//   group               : [group_id 4B] + [nonce_ctr 8B] + [tag 16B] = 28 B
constexpr size_t CRYPTO_N2N_OVERHEAD   = CRYPTO_NONCE_CTR_SIZE + CRYPTO_TAG_SIZE;      // 24
constexpr size_t CRYPTO_GRP_OVERHEAD   = 4 + CRYPTO_NONCE_CTR_SIZE + CRYPTO_TAG_SIZE;  // 28

enum class CryptoError : int8_t {
    OK          =  0,
    NO_KEY      = -1,   // Key not provisioned for this peer/group
    AUTH_FAIL   = -2,   // Poly1305 tag mismatch
    REPLAY      = -3,   // Nonce replay detected
    BUF_SMALL   = -4,   // Output buffer too small / plaintext too large
    MBEDTLS_ERR = -5,   // Underlying mbedTLS failure
    BAD_LEN     = -6,   // Payload too short to contain header+tag
};

} // namespace lib::crypto
