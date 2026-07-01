#include "ReplayProtection.hh"
#include <cstring>

namespace lib::crypto {

ReplayProtection::ReplayProtection()
{
    _mutex = xSemaphoreCreateMutex();
    memset(_senders, 0, sizeof(_senders));
}

bool ReplayProtection::accept(uint32_t sender_id, uint64_t counter)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);

    int slot = -1;
    for (int i = 0; i < CRYPTO_MAX_REPLAY_SENDERS; i++) {
        if (_senders[i].valid && _senders[i].sender_id == sender_id) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        // New sender: find empty slot or evict slot 0 (simple LRU)
        for (int i = 0; i < CRYPTO_MAX_REPLAY_SENDERS; i++) {
            if (!_senders[i].valid) { slot = i; break; }
        }
        if (slot < 0) slot = 0;
        _senders[slot] = { sender_id, 0, true };
    }

    SenderEntry &e = _senders[slot];
    bool ok = false;

    if (counter > e.max_counter) {
        e.max_counter = counter;
        ok = true;
    } else if (e.max_counter - counter < REPLAY_WINDOW) {
        // Within the window --> out-of-order but not a replay
        ok = true;
    }

    xSemaphoreGive(_mutex);
    return ok;
}

void ReplayProtection::reset(uint32_t sender_id)
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (int i = 0; i < CRYPTO_MAX_REPLAY_SENDERS; i++) {
        if (_senders[i].valid && _senders[i].sender_id == sender_id) {
            memset(&_senders[i], 0, sizeof(SenderEntry));
            break;
        }
    }
    xSemaphoreGive(_mutex);
}

void ReplayProtection::reset_all()
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memset(_senders, 0, sizeof(_senders));
    xSemaphoreGive(_mutex);
}

} // namespace lib::crypto
