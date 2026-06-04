#include "CryptoManager.hh"
#include "AESGCM.hh"
#include <cstring>
#include <config.h>

namespace lib::crypto {

// ── AAD helpers ─────────────────────────────────────────────────────────────
// Bind ciphertext to immutable routing fields so re-routing attacks fail.

static void build_aad_n2n(uint16_t pkt_id, uint32_t src, uint32_t dst,
                           uint8_t aad[10])
{
    aad[0] = pkt_id & 0xFF;       aad[1] = pkt_id >> 8;
    aad[2] = (src >>  0) & 0xFF;  aad[3] = (src >>  8) & 0xFF;
    aad[4] = (src >> 16) & 0xFF;  aad[5] = (src >> 24) & 0xFF;
    aad[6] = (dst >>  0) & 0xFF;  aad[7] = (dst >>  8) & 0xFF;
    aad[8] = (dst >> 16) & 0xFF;  aad[9] = (dst >> 24) & 0xFF;
}

static void build_aad_grp(uint16_t pkt_id, uint32_t src, uint32_t group_id,
                           uint8_t aad[10])
{
    aad[0] = pkt_id & 0xFF;           aad[1] = pkt_id >> 8;
    aad[2] = (src      >>  0) & 0xFF; aad[3] = (src      >>  8) & 0xFF;
    aad[4] = (src      >> 16) & 0xFF; aad[5] = (src      >> 24) & 0xFF;
    aad[6] = (group_id >>  0) & 0xFF; aad[7] = (group_id >>  8) & 0xFF;
    aad[8] = (group_id >> 16) & 0xFF; aad[9] = (group_id >> 24) & 0xFF;
}

// ── Private helpers ──────────────────────────────────────────────────────────

CryptoError CryptoManager::_encrypt_with_key(
    uint32_t my_id,
    const uint8_t key[CRYPTO_KEY_SIZE],
    const uint8_t *plaintext, size_t pt_len,
    const uint8_t *aad, size_t aad_len,
    uint8_t *out_payload, uint8_t &out_len)
{
    if (CRYPTO_N2N_OVERHEAD + pt_len > A3MESH_MAX_PAYLOAD)
        return CryptoError::BUF_SMALL;

    uint8_t full_nonce[CRYPTO_NONCE_SIZE];
    nonce.next_nonce(my_id, full_nonce);

    // Store the 8-byte counter at the start of the output payload
    memcpy(out_payload, full_nonce + 4, CRYPTO_NONCE_CTR_SIZE);

    uint8_t tag[CRYPTO_TAG_SIZE];
    CryptoError err = AESGCM::encrypt(
        key, full_nonce, aad, aad_len,
        plaintext, pt_len,
        out_payload + CRYPTO_NONCE_CTR_SIZE + CRYPTO_TAG_SIZE,
        tag);

    if (err != CryptoError::OK)
        return err;

    memcpy(out_payload + CRYPTO_NONCE_CTR_SIZE, tag, CRYPTO_TAG_SIZE);
    out_len = (uint8_t)(CRYPTO_NONCE_CTR_SIZE + CRYPTO_TAG_SIZE + pt_len);
    return CryptoError::OK;
}

CryptoError CryptoManager::_decrypt_with_key(
    uint32_t sender_id,
    const uint8_t key[CRYPTO_KEY_SIZE],
    const uint8_t *payload, size_t payload_len,
    const uint8_t *aad, size_t aad_len,
    uint8_t *out, size_t out_cap,
    size_t &out_len)
{
    if (payload_len < CRYPTO_N2N_OVERHEAD)
        return CryptoError::BAD_LEN;

    size_t ct_len = payload_len - CRYPTO_N2N_OVERHEAD;
    if (ct_len > out_cap)
        return CryptoError::BUF_SMALL;

    const uint8_t *nonce_ctr = payload;
    const uint8_t *tag       = payload + CRYPTO_NONCE_CTR_SIZE;
    const uint8_t *ct        = payload + CRYPTO_NONCE_CTR_SIZE + CRYPTO_TAG_SIZE;

    uint64_t ctr = 0;
    for (int i = 0; i < 8; i++)
        ctr |= (uint64_t)nonce_ctr[i] << (8 * i);

    if (!replay.accept(sender_id, ctr))
        return CryptoError::REPLAY;

    uint8_t full_nonce[CRYPTO_NONCE_SIZE];
    NonceManager::build_nonce(sender_id, nonce_ctr, full_nonce);

    CryptoError err = AESGCM::decrypt(
        key, full_nonce, aad, aad_len, tag, ct, ct_len, out);

    if (err != CryptoError::OK)
        return err;

    out_len = ct_len;
    return CryptoError::OK;
}

// ── TX ───────────────────────────────────────────────────────────────────────

CryptoError CryptoManager::encrypt_n2n(
    uint32_t my_id, uint32_t dst,
    const uint8_t *plaintext, size_t pt_len,
    lib::packet::A3MESH_packet_t &pkt)
{
    uint8_t key[CRYPTO_KEY_SIZE];
    if (!keys.get_peer_key(dst, key))
        return CryptoError::NO_KEY;

    uint8_t aad[10];
    build_aad_n2n(pkt.pkt_id, my_id, dst, aad);

    return _encrypt_with_key(my_id, key, plaintext, pt_len,
                             aad, sizeof(aad),
                             pkt.payload, pkt.payload_len);
}

CryptoError CryptoManager::encrypt_net(
    uint32_t my_id,
    const uint8_t *plaintext, size_t pt_len,
    lib::packet::A3MESH_packet_t &pkt)
{
    uint8_t key[CRYPTO_KEY_SIZE];
    if (!keys.get_net_key(key))
        return CryptoError::NO_KEY;

    uint8_t aad[10];
    build_aad_n2n(pkt.pkt_id, my_id, pkt.dst, aad);

    return _encrypt_with_key(my_id, key, plaintext, pt_len,
                             aad, sizeof(aad),
                             pkt.payload, pkt.payload_len);
}

CryptoError CryptoManager::encrypt_grp(
    uint32_t my_id, uint32_t group_id,
    const uint8_t *plaintext, size_t pt_len,
    lib::packet::A3MESH_packet_t &pkt)
{
    if (4 + CRYPTO_N2N_OVERHEAD + pt_len > A3MESH_MAX_PAYLOAD)
        return CryptoError::BUF_SMALL;

    uint8_t key[CRYPTO_KEY_SIZE];
    if (!keys.get_group_key(group_id, key))
        return CryptoError::NO_KEY;

    // Prepend group_id (4 bytes LE)
    pkt.payload[0] = (group_id >>  0) & 0xFF;
    pkt.payload[1] = (group_id >>  8) & 0xFF;
    pkt.payload[2] = (group_id >> 16) & 0xFF;
    pkt.payload[3] = (group_id >> 24) & 0xFF;

    uint8_t aad[10];
    build_aad_grp(pkt.pkt_id, my_id, group_id, aad);

    uint8_t inner_len = 0;
    CryptoError err = _encrypt_with_key(my_id, key, plaintext, pt_len,
                                        aad, sizeof(aad),
                                        pkt.payload + 4, inner_len);
    if (err != CryptoError::OK)
        return err;

    pkt.payload_len = 4 + inner_len;
    return CryptoError::OK;
}

// ── RX ───────────────────────────────────────────────────────────────────────

CryptoError CryptoManager::decrypt_n2n(
    uint32_t sender_id,
    lib::packet::A3MESH_packet_t &pkt,
    uint8_t *out_buf, size_t out_cap)
{
    uint8_t key[CRYPTO_KEY_SIZE];
    if (!keys.get_peer_key(sender_id, key))
        return CryptoError::NO_KEY;

    uint8_t aad[10];
    build_aad_n2n(pkt.pkt_id, sender_id, pkt.dst, aad);

    size_t out_len = 0;
    CryptoError err = _decrypt_with_key(sender_id, key,
                                        pkt.payload, pkt.payload_len,
                                        aad, sizeof(aad),
                                        out_buf, out_cap, out_len);
    if (err == CryptoError::OK) {
        memcpy(pkt.payload, out_buf, out_len);
        pkt.payload_len = (uint8_t)out_len;
    }
    return err;
}

CryptoError CryptoManager::decrypt_net(
    uint32_t sender_id,
    lib::packet::A3MESH_packet_t &pkt,
    uint8_t *out_buf, size_t out_cap)
{
    uint8_t key[CRYPTO_KEY_SIZE];
    if (!keys.get_net_key(key))
        return CryptoError::NO_KEY;

    uint8_t aad[10];
    build_aad_n2n(pkt.pkt_id, sender_id, pkt.dst, aad);

    size_t out_len = 0;
    CryptoError err = _decrypt_with_key(sender_id, key,
                                        pkt.payload, pkt.payload_len,
                                        aad, sizeof(aad),
                                        out_buf, out_cap, out_len);
    if (err == CryptoError::OK) {
        memcpy(pkt.payload, out_buf, out_len);
        pkt.payload_len = (uint8_t)out_len;
    }
    return err;
}

CryptoError CryptoManager::decrypt_grp(
    uint32_t sender_id,
    lib::packet::A3MESH_packet_t &pkt,
    uint8_t *out_buf, size_t out_cap,
    uint32_t &out_group_id)
{
    if (pkt.payload_len < 4 + (uint8_t)CRYPTO_N2N_OVERHEAD)
        return CryptoError::BAD_LEN;

    out_group_id =
        ((uint32_t)pkt.payload[0])        |
        ((uint32_t)pkt.payload[1] <<  8)  |
        ((uint32_t)pkt.payload[2] << 16)  |
        ((uint32_t)pkt.payload[3] << 24);

    uint8_t key[CRYPTO_KEY_SIZE];
    if (!keys.get_group_key(out_group_id, key))
        return CryptoError::NO_KEY;

    uint8_t aad[10];
    build_aad_grp(pkt.pkt_id, sender_id, out_group_id, aad);

    size_t out_len = 0;
    CryptoError err = _decrypt_with_key(sender_id, key,
                                        pkt.payload + 4, pkt.payload_len - 4,
                                        aad, sizeof(aad),
                                        out_buf, out_cap, out_len);
    if (err == CryptoError::OK) {
        memcpy(pkt.payload, out_buf, out_len);
        pkt.payload_len = (uint8_t)out_len;
    }
    return err;
}

} // namespace lib::crypto
