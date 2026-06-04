#include "AESGCM.hh"
#include "mbedtls/gcm.h"

namespace lib::crypto {

CryptoError AESGCM::encrypt(
    const uint8_t  key[CRYPTO_KEY_SIZE],
    const uint8_t  nonce[CRYPTO_NONCE_SIZE],
    const uint8_t *aad,       size_t aad_len,
    const uint8_t *plaintext, size_t pt_len,
    uint8_t       *ciphertext,
    uint8_t        tag[CRYPTO_TAG_SIZE])
{
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES,
                                  key, CRYPTO_KEY_SIZE * 8u);
    if (ret == 0)
        ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
                                         pt_len, nonce, CRYPTO_NONCE_SIZE,
                                         aad, aad_len,
                                         plaintext, ciphertext,
                                         CRYPTO_TAG_SIZE, tag);
    mbedtls_gcm_free(&ctx);
    return (ret == 0) ? CryptoError::OK : CryptoError::MBEDTLS_ERR;
}

CryptoError AESGCM::decrypt(
    const uint8_t  key[CRYPTO_KEY_SIZE],
    const uint8_t  nonce[CRYPTO_NONCE_SIZE],
    const uint8_t *aad,        size_t aad_len,
    const uint8_t  tag[CRYPTO_TAG_SIZE],
    const uint8_t *ciphertext, size_t ct_len,
    uint8_t       *plaintext)
{
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES,
                                  key, CRYPTO_KEY_SIZE * 8u);
    if (ret == 0)
        ret = mbedtls_gcm_auth_decrypt(&ctx, ct_len,
                                        nonce, CRYPTO_NONCE_SIZE,
                                        aad, aad_len,
                                        tag, CRYPTO_TAG_SIZE,
                                        ciphertext, plaintext);
    mbedtls_gcm_free(&ctx);

    if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED)
        return CryptoError::AUTH_FAIL;
    return (ret == 0) ? CryptoError::OK : CryptoError::MBEDTLS_ERR;
}

} // namespace lib::crypto
