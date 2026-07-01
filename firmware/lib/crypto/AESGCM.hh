#pragma once
#include "CryptoTypes.hh"

namespace lib::crypto {

/*!
 * \brief Thin stateless wrapper around ESP32 hardware-accelerated AES-256-GCM.
 *
 * Provides AEAD (Authenticated Encryption with Associated Data) using:
 *   - 256-bit key (32 bytes)
 *   - 96-bit nonce (12 bytes, RFC 5116)
 *   - 128-bit authentication tag (16 bytes)
 *
 * The mbedTLS context is stack-allocated per call --> no concurrency issues.
 * Internally delegates to mbedtls_gcm_* which maps to esp_aes_gcm_* (HW).
 */
class AESGCM {
public:
    /*!
     * \brief Encrypt plaintext and produce an authentication tag.
     *
     * \param key        32-byte AES-256 key.
     * \param nonce      12-byte nonce (must be unique per (key, message) pair).
     * \param aad        Additional Authenticated Data (NULL if aad_len==0).
     * \param aad_len    Length of AAD in bytes.
     * \param plaintext  Input buffer.
     * \param pt_len     Input length in bytes.
     * \param ciphertext Output buffer (must hold pt_len bytes).
     * \param tag        Output: 16-byte GCM authentication tag.
     *
     * \retval CryptoError::OK          on success.
     * \retval CryptoError::MBEDTLS_ERR on mbedTLS/hardware failure.
     */
    static CryptoError encrypt(
        const uint8_t  key[CRYPTO_KEY_SIZE],
        const uint8_t  nonce[CRYPTO_NONCE_SIZE],
        const uint8_t *aad,        size_t aad_len,
        const uint8_t *plaintext,  size_t pt_len,
        uint8_t       *ciphertext,
        uint8_t        tag[CRYPTO_TAG_SIZE]);

    /*!
     * \brief Authenticate and decrypt a ciphertext.
     *
     * \param key        32-byte AES-256 key.
     * \param nonce      12-byte nonce (must match the one used for encryption).
     * \param aad        Additional Authenticated Data.
     * \param aad_len    Length of AAD.
     * \param tag        16-byte GCM tag to verify.
     * \param ciphertext Input buffer.
     * \param ct_len     Input length in bytes.
     * \param plaintext  Output buffer (must hold ct_len bytes).
     *
     * \retval CryptoError::OK        success and tag matches.
     * \retval CryptoError::AUTH_FAIL tag mismatch (discard output).
     * \retval CryptoError::MBEDTLS_ERR other failure.
     */
    static CryptoError decrypt(
        const uint8_t  key[CRYPTO_KEY_SIZE],
        const uint8_t  nonce[CRYPTO_NONCE_SIZE],
        const uint8_t *aad,        size_t aad_len,
        const uint8_t  tag[CRYPTO_TAG_SIZE],
        const uint8_t *ciphertext, size_t ct_len,
        uint8_t       *plaintext);
};

} // namespace lib::crypto
