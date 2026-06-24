#pragma once
#include <config.h>
#include <stdint.h>
#include <atomic>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace lib::power
{
    /*!
     * \brief Adaptive CPU frequency manager for the A3MESH node.
     *
     * When no LoRa mesh activity (discovery or data) is detected for
     * POWER_IDLE_TIMEOUT_MS milliseconds the CPU clock is reduced to
     * POWER_IDLE_MHZ.  The clock is restored to POWER_ACTIVE_MHZ
     * immediately upon the next call to notify_activity().
     *
     * BLE and LoRa SPI continue to operate correctly at 80 MHz.
     */
    class PowerManager
    {
    public:
        PowerManager();

        /*!
         * \brief Signal that mesh activity was detected (call on every valid
         *        received LoRa packet).  Thread-safe; may be called from any task.
         */
        void notify_activity();

        /*!
         * \brief Create and start the FreeRTOS monitoring task.
         * \retval true   Task created successfully.
         * \retval false  Task creation failed (out of heap).
         */
        bool start_task();

    private:
        std::atomic<uint32_t> _last_activity_ms;
        std::atomic<bool>     _is_idle;

        static void power_task(void *pvParameters);
    };

} // namespace lib::power
