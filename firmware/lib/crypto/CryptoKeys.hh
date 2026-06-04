#pragma once
#include "CryptoTypes.hh"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define CRYPTO_MAX_PEER_KEYS  8
#define CRYPTO_MAX_GROUP_KEYS 4

namespace lib::crypto {

struct PeerKey {
    uint32_t peer_id;
    uint8_t  key[CRYPTO_KEY_SIZE];
    bool     valid;
};

struct GroupKey {
    uint32_t group_id;
    uint8_t  key[CRYPTO_KEY_SIZE];
    bool     valid;
};

/*!
 * \brief Thread-safe key store for all A3MESH security domains.
 *
 * Holds one network key (shared by all mesh nodes), up to
 * CRYPTO_MAX_PEER_KEYS node-to-node session keys, and up to
 * CRYPTO_MAX_GROUP_KEYS group keys.  All operations are protected
 * by an internal FreeRTOS mutex.
 */
class CryptoKeys {
public:
    CryptoKeys();

    // --- Network key (shared mesh-wide key) ---
    bool set_net_key(const uint8_t key[CRYPTO_KEY_SIZE]);
    bool get_net_key(uint8_t out[CRYPTO_KEY_SIZE]) const;
    bool has_net_key() const;
    void clear_net_key();

    // --- Node-to-node peer keys ---
    bool set_peer_key(uint32_t peer_id, const uint8_t key[CRYPTO_KEY_SIZE]);
    bool get_peer_key(uint32_t peer_id, uint8_t out[CRYPTO_KEY_SIZE]) const;
    bool has_peer_key(uint32_t peer_id) const;
    void clear_peer_key(uint32_t peer_id);

    // --- Group keys ---
    bool set_group_key(uint32_t group_id, const uint8_t key[CRYPTO_KEY_SIZE]);
    bool get_group_key(uint32_t group_id, uint8_t out[CRYPTO_KEY_SIZE]) const;
    bool has_group_key(uint32_t group_id) const;
    void clear_group_key(uint32_t group_id);

    void clear_all();

private:
    mutable SemaphoreHandle_t _mutex;

    bool    _net_key_valid;
    uint8_t _net_key[CRYPTO_KEY_SIZE];

    PeerKey  _peer_keys[CRYPTO_MAX_PEER_KEYS];
    GroupKey _group_keys[CRYPTO_MAX_GROUP_KEYS];
};

} // namespace lib::crypto
