#include "LoRaDriver.h"
#include <config.h>

namespace lib::lora
{
    LoRaDriver::LoRaDriver()
    {
        this->radio = nullptr;
        receivedFlag = false;
    }

    bool LoRaDriver::begin(const lora::A3MESHConfig &config)
    {
        ESP_LOGV(A3M_TAG, "Initializing A3MESH v%s", A3M_VERSION);

        this->config = config;

        // Set module configuration
        this->radio = new SX1262(new Module(
            config.cs_pin,
            config.irq_pin,
            config.rst_pin,
            config.dio1_pin));

        // Initialize the radio with the specified configuration
        int16_t state = this->radio->begin(
            config.frequency,
            config.bandwidth,
            config.sf,
            config.cr,
            config.sync_word,
            config.power,
            config.preamble_len);


        return state == RADIOLIB_ERR_NONE;
    }

    bool LoRaDriver::sendRawPacket(uint8_t* data, size_t len)
    {
        if (len > A3MESH_MAX_PACKET_SIZE)
            return false;

        int16_t state = this->radio->transmit(data, len);
        Serial.println("Raw packet sent!");
        return state == RADIOLIB_ERR_NONE;
    }

    bool LoRaDriver::sendPacket(const lib::packet::A3MESHPacket_t &packet)
    {
        uint8_t buffer[A3MESH_MAX_PACKET_SIZE];
        size_t total_len = 0;

        // serialize packet
        buffer[total_len++] = packet.pkt_id;
        buffer[total_len++] = packet.src;
        buffer[total_len++] = packet.node_id;
        buffer[total_len++] = packet.dst;
        buffer[total_len++] = packet.ttl;
        buffer[total_len++] = packet.type;
        buffer[total_len++] = packet.flags;
        buffer[total_len++] = packet.payload_len;
        memcpy(&buffer[total_len], packet.payload, packet.payload_len);

        total_len += packet.payload_len;

        if (packet.payload_len > A3MESH_MAX_PAYLOAD)
            return false;

        int16_t state = this->radio->transmit(buffer, total_len);

        if (state != RADIOLIB_ERR_NONE)
            return false;

        Serial.println("Packet sent!");
        Serial.print("Payload: ");
        for (size_t i = 0; i < packet.payload_len; i++)
        {
            Serial.print((char)packet.payload[i]);
        }
        Serial.println();

        return true;
    }

    volatile bool LoRaDriver::receivedFlag = false;

    bool LoRaDriver::startReceive()
    {
        int16_t state = this->radio->startReceive();

        return state == RADIOLIB_ERR_NONE;
    }

    void LoRaDriver::clearDioActions()
    {
        this->radio->clearDio1Action();
    }

    void LoRaDriver::setPacketReceivedAction()
    {
        this->radio->setPacketReceivedAction([]() {
            receivedFlag = true;
        });
    }

    bool LoRaDriver::checkPacketReceived()
    {
        return this->receivedFlag;
    }

    int LoRaDriver::readData(uint8_t* buffer, size_t len)
    {
        if (!this->receivedFlag)
            return 0;

        this->receivedFlag = false;

        int packetSize = this->radio->getPacketLength();

        if (packetSize == 0)
            return 0;

        if (packetSize > len)
            packetSize = len;

        int state = this->radio->readData(buffer, packetSize);

        this->radio->startReceive();

        if (state != RADIOLIB_ERR_NONE)
            return -1;

        return packetSize;
    }

} // namespace lib::lora