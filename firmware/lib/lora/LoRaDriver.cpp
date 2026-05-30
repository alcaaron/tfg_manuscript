#include "LoRaDriver.hh"

namespace lib::lora
{
    volatile bool LoRaDriver::lora_rx_flag = false;

    LoRaDriver::rx_LoRa_msg_t LoRaDriver::pool[8];
    volatile uint8_t LoRaDriver::pool_idx = 0;

    LoRaDriver::LoRaDriver() {}



    bool LoRaDriver::begin(const lora::A3MESHConfig &config)
    {
        // Set LoRa pins
        LoRa.setPins(config.nss_pin, config.rst_pin, config.dio0_pin);

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

    bool LoRaDriver::send_raw_pkt(const uint8_t &data, size_t len)
    {
        if (len > LORA_MAX_PACKET_SIZE)
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
        lora_rx_flag = true;
    }

    bool LoRaDriver::consume_rx_flag()
    {
        if (!lora_rx_flag)
            return false;

        lora_rx_flag = false;
        return true;
    }

    LoRaDriver::rx_LoRa_msg_t* LoRaDriver::alloc_frame()
    {
        rx_LoRa_msg_t* f = &pool[pool_idx];
        pool_idx = (pool_idx + 1) & 7;
        return f;
    }

} // namespace lib::lora