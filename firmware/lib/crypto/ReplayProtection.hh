#pragma once
#include "CryptoTypes.hh"
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define CRYPTO_MAX_REPLAY_SENDERS 16

namespace lib::crypto {

/*!
 * \brief Per-sender sliding-window replay protection.
 *
 * Accepts a (sender_id, counter) pair if:
 *   - counter > max_counter_seen_for_sender  (strictly newer), OR
 *   - counter is within a REPLAY_WINDOW behind the max (out-of-order delivery).
 *
 * Packets with counter <= (max - REPLAY_WINDOW) are rejected as replays.
 * Thread-safe; uses an internal FreeRTOS mutex.
 */
class ReplayProtection {
public:
    ReplayProtection();

    /*!
     * \brief Accept or reject a nonce counter from a given sender.
     *
     * Updates internal state when accepting.
     *
     * \param sender_id  pkt.src of the received frame.
     * \param counter    64-bit nonce counter parsed from the payload.
     *
     * \retval true   Packet is acceptable (not a replay).
     * \retval false  Packet is a replay and must be dropped.
     */
    bool accept(uint32_t sender_id, uint64_t counter);

    void reset(uint32_t sender_id);
    void reset_all();

private:
    static constexpr uint64_t REPLAY_WINDOW = 64;

    struct SenderEntry {
        uint32_t sender_id;
        uint64_t max_counter;
        bool     valid;
    };

    SemaphoreHandle_t _mutex;
    SenderEntry _senders[CRYPTO_MAX_REPLAY_SENDERS];
};

} // namespace lib::crypto
