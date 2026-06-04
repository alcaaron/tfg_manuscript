#include "CryptoKeys.hh"
#include <cstring>

namespace lib::crypto {

CryptoKeys::CryptoKeys() : _net_key_valid(false)
{
    _mutex = xSemaphoreCreateMutex();
    memset(_net_key,    0, sizeof(_net_key));
    memset(_peer_keys,  0, sizeof(_peer_keys));
    memset(_group_keys, 0, sizeof(_group_keys));
}

// ---- Network key ----

bool CryptoKeys::set_net_key(const uint8_t key[CRYPTO_KEY_SIZE]) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memcpy(_net_key, key, CRYPTO_KEY_SIZE);
    _net_key_valid = true;
    xSemaphoreGive(_mutex);
    return true;
}

bool CryptoKeys::get_net_key(uint8_t out[CRYPTO_KEY_SIZE]) const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool v = _net_key_valid;
    if (v) memcpy(out, _net_key, CRYPTO_KEY_SIZE);
    xSemaphoreGive(_mutex);
    return v;
}

bool CryptoKeys::has_net_key() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool v = _net_key_valid;
    xSemaphoreGive(_mutex);
    return v;
}

void CryptoKeys::clear_net_key() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memset(_net_key, 0, CRYPTO_KEY_SIZE);
    _net_key_valid = false;
    xSemaphoreGive(_mutex);
}

// ---- Peer keys ----

bool CryptoKeys::set_peer_key(uint32_t peer_id, const uint8_t key[CRYPTO_KEY_SIZE]) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_PEER_KEYS; i++) {
        if (_peer_keys[i].valid && _peer_keys[i].peer_id == peer_id) {
            memcpy(_peer_keys[i].key, key, CRYPTO_KEY_SIZE);
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    for (int i = 0; i < CRYPTO_MAX_PEER_KEYS; i++) {
        if (!_peer_keys[i].valid) {
            _peer_keys[i] = { peer_id, {}, true };
            memcpy(_peer_keys[i].key, key, CRYPTO_KEY_SIZE);
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    xSemaphoreGive(_mutex);
    return false; // table full
}

bool CryptoKeys::get_peer_key(uint32_t peer_id, uint8_t out[CRYPTO_KEY_SIZE]) const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_PEER_KEYS; i++) {
        if (_peer_keys[i].valid && _peer_keys[i].peer_id == peer_id) {
            memcpy(out, _peer_keys[i].key, CRYPTO_KEY_SIZE);
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    xSemaphoreGive(_mutex);
    return false;
}

bool CryptoKeys::has_peer_key(uint32_t peer_id) const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_PEER_KEYS; i++) {
        if (_peer_keys[i].valid && _peer_keys[i].peer_id == peer_id) {
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    xSemaphoreGive(_mutex);
    return false;
}

void CryptoKeys::clear_peer_key(uint32_t peer_id) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_PEER_KEYS; i++) {
        if (_peer_keys[i].valid && _peer_keys[i].peer_id == peer_id) {
            memset(&_peer_keys[i], 0, sizeof(PeerKey));
            break;
        }
    }
    xSemaphoreGive(_mutex);
}

// ---- Group keys ----

bool CryptoKeys::set_group_key(uint32_t group_id, const uint8_t key[CRYPTO_KEY_SIZE]) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_GROUP_KEYS; i++) {
        if (_group_keys[i].valid && _group_keys[i].group_id == group_id) {
            memcpy(_group_keys[i].key, key, CRYPTO_KEY_SIZE);
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    for (int i = 0; i < CRYPTO_MAX_GROUP_KEYS; i++) {
        if (!_group_keys[i].valid) {
            _group_keys[i] = { group_id, {}, true };
            memcpy(_group_keys[i].key, key, CRYPTO_KEY_SIZE);
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    xSemaphoreGive(_mutex);
    return false;
}

bool CryptoKeys::get_group_key(uint32_t group_id, uint8_t out[CRYPTO_KEY_SIZE]) const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_GROUP_KEYS; i++) {
        if (_group_keys[i].valid && _group_keys[i].group_id == group_id) {
            memcpy(out, _group_keys[i].key, CRYPTO_KEY_SIZE);
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    xSemaphoreGive(_mutex);
    return false;
}

bool CryptoKeys::has_group_key(uint32_t group_id) const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_GROUP_KEYS; i++) {
        if (_group_keys[i].valid && _group_keys[i].group_id == group_id) {
            xSemaphoreGive(_mutex);
            return true;
        }
    }
    xSemaphoreGive(_mutex);
    return false;
}

void CryptoKeys::clear_group_key(uint32_t group_id) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_GROUP_KEYS; i++) {
        if (_group_keys[i].valid && _group_keys[i].group_id == group_id) {
            memset(&_group_keys[i], 0, sizeof(GroupKey));
            break;
        }
    }
    xSemaphoreGive(_mutex);
}

void CryptoKeys::clear_all() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memset(_net_key, 0, sizeof(_net_key));
    _net_key_valid = false;
    memset(_peer_keys,  0, sizeof(_peer_keys));
    memset(_group_keys, 0, sizeof(_group_keys));
    xSemaphoreGive(_mutex);
}

} // namespace lib::crypto
