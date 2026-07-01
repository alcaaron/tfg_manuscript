#include "PowerManager.hh"

namespace lib::power
{
    PowerManager::PowerManager()
        : _last_activity_ms(0), _is_idle(false)
    {
    }

    void PowerManager::notify_activity()
    {
        _last_activity_ms.store(millis(), std::memory_order_relaxed);

        // Only act on the idle→active transition to avoid calling the
        // frequency API on every packet.
        if (_is_idle.exchange(false, std::memory_order_acq_rel))
        {
            setCpuFrequencyMhz(POWER_ACTIVE_MHZ);
            MESH_LOG("[PWR] Activity --> CPU restored to %d MHz\r\n", POWER_ACTIVE_MHZ);
        }
    }

    bool PowerManager::start_task()
    {
        _last_activity_ms.store(millis(), std::memory_order_relaxed);
        return xTaskCreatePinnedToCore(
            vPowerTask, "PowerTask", 2048, this, 1, NULL, 1) == pdPASS;
    }

    void PowerManager::vPowerTask(void *pvParameters)
    {
        auto *self = static_cast<PowerManager *>(pvParameters);

        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(POWER_CHECK_INTERVAL_MS));

            if (!self->_is_idle.load(std::memory_order_relaxed))
            {
                uint32_t elapsed = millis() -
                    self->_last_activity_ms.load(std::memory_order_relaxed);

                if (elapsed >= POWER_IDLE_TIMEOUT_MS)
                {
                    self->_is_idle.store(true, std::memory_order_release);
                    setCpuFrequencyMhz(POWER_IDLE_MHZ);
                    MESH_LOG("[PWR] Idle (%lu s) --> CPU reduced to %d MHz\r\n",
                             elapsed / 1000UL, POWER_IDLE_MHZ);
                }
            }
        }
    }

} // namespace lib::power
