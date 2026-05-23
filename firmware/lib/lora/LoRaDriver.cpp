#include "LoRaDriver.hh"

namespace lib::lora
{
    LoRaDriver::LoRaDriver() {}

    bool LoRaDriver::begin(const lora::A3MESHConfig &config)
    {
        // Set LoRa pins
        LoRa.setPins(config.nss_pin, config.rst_pin, config.dio0_pin);

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
        LoRa.endPacket(true);
        Serial.printf("[LORA] Sent packet with %d bytes\n", bytesWritten);

        LoRa.receive();

        return bytesWritten > 0;
    }

} // namespace lib::lora