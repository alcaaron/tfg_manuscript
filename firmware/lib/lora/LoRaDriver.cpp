#include "LoRaDriver.hh"

namespace lib::lora
{
    SemaphoreHandle_t LoRaDriver::_rx_sem = nullptr;

    LoRaDriver::rx_LoRa_msg_t LoRaDriver::pool[8];
    volatile uint8_t LoRaDriver::pool_idx = 0;

    LoRaDriver::LoRaDriver() {}



    bool LoRaDriver::begin(const lora::A3MESHConfig &config)
    {
        // Set LoRa pins
        LoRa.setPins(config.nss_pin, config.rst_pin, config.dio0_pin);

        _rx_sem = xSemaphoreCreateBinary();
        attachInterrupt(digitalPinToInterrupt(config.dio0_pin), onLoRaRxIRQ, RISING);

        // LoRa Module Reset
        pinMode(config.rst_pin, OUTPUT);
        digitalWrite(config.rst_pin, LOW);
        delay(10);
        digitalWrite(config.rst_pin, HIGH);
        delay(10);

        // LoRa Initialization
        if (!LoRa.begin(config.frequency)) { return false; }

        // LoRa Configuration
        LoRa.setSpreadingFactor(config.sf);
        LoRa.setSignalBandwidth(config.bandwidth);
        LoRa.setCodingRate4(config.cr);
        LoRa.setPreambleLength(config.preamble_len);
        LoRa.setTxPower(config.power);

        LoRa.receive();

        return true;
    }

    // LoRa SX127x time-on-air formula (Semtech AN1200.13 rev2, §4).
    // CRC=1, explicit header (IH=0), LDRO off (T_sym=8.19ms @ SF11/250kHz < 16ms threshold).
    uint32_t LoRaDriver::compute_airtime_ms(size_t pkt_len)
    {
        constexpr uint32_t bw_hz    = static_cast<uint32_t>(BANDWIDTH_250_KHZ);
        constexpr uint8_t  sf       = A3MESH_SF;
        constexpr uint8_t  preamble = A3MESH_PREAMBLE_LENGTH;
        constexpr uint8_t  cr       = A3MESH_CR;  // 1 → 4/5

        // T_sym in µs = 2^SF * 1_000_000 / BW
        const uint32_t t_sym_us = ((uint32_t)1u << sf) * 1000000UL / bw_hz;

        // T_preamble = (preamble + 4.25) * T_sym  → use *(4*p+17)/4 to stay integer
        const uint32_t t_pre_us = (4u * (uint32_t)preamble + 17u) * t_sym_us / 4u;

        // n_payload_symb = 8 + max(0, ceil((8*PL - 4*SF + 44) / (4*SF)) * (CR+4))
        const int32_t num   = 8 * (int32_t)pkt_len - 4 * (int32_t)sf + 44;
        const int32_t denom = 4 * (int32_t)sf;
        const int32_t ceil_part = (num > 0) ? ((num + denom - 1) / denom) : 0;
        const uint32_t n_payload_symb = 8u + (uint32_t)(ceil_part * (int32_t)(cr + 4));

        const uint32_t t_pkt_us = t_pre_us + n_payload_symb * t_sym_us;

        return (t_pkt_us + 999u) / 1000u;  // µs → ms, ceiling
    }

    bool LoRaDriver::check_and_record_airtime(uint32_t airtime_ms)
    {
        const uint32_t now = millis();
        uint32_t used = 0;

        for (uint8_t i = 0; i < LORA_AIRTIME_RECORDS; i++) {
            if (_airtime_log[i].dur_ms > 0 &&
                (now - _airtime_log[i].ts_ms) < LORA_DUTY_CYCLE_WINDOW_MS) {
                used += _airtime_log[i].dur_ms;
            }
        }

        if (used + airtime_ms > LORA_DUTY_CYCLE_MAX_MS) {
            MESH_LOG("[LORA] Duty cycle limit reached (%lu/%lu ms used), TX dropped\r\n",
                     used, LORA_DUTY_CYCLE_MAX_MS);
            return false;
        }

        _airtime_log[_airtime_log_idx].ts_ms  = now;
        _airtime_log[_airtime_log_idx].dur_ms  = airtime_ms;
        _airtime_log_idx = (_airtime_log_idx + 1) % LORA_AIRTIME_RECORDS;

        return true;
    }

    bool LoRaDriver::send_raw_pkt(const uint8_t &data, size_t len)
    {
        if (len > LORA_MAX_PACKET_SIZE)
            return false;

        const uint32_t airtime_ms = compute_airtime_ms(len);

        if (!check_and_record_airtime(airtime_ms))
            return false;

        LoRa.idle();

        LoRa.beginPacket();
        size_t bytesWritten = LoRa.write(&data, len);
        LoRa.endPacket(false);
        LoRa.receive();

        return bytesWritten > 0;
    }

    void IRAM_ATTR LoRaDriver::onLoRaRxIRQ()
    {
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(_rx_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }

    void LoRaDriver::wait_for_rx()
    {
        xSemaphoreTake(_rx_sem, portMAX_DELAY);
    }

    LoRaDriver::rx_LoRa_msg_t* LoRaDriver::alloc_frame()
    {
        rx_LoRa_msg_t* f = &pool[pool_idx];
        pool_idx = (pool_idx + 1) & 7;
        return f;
    }

} // namespace lib::lora