#pragma once

#include <RadioLib.h>
#include "LoRaConfig.h"
#include "A3MESHPacket.h"

namespace lib::lora
{
    class LoRaDriver
    {
    public:
        LoRaDriver();

        bool begin(const lora::A3MESHConfig &config);

        bool initTasks();

        bool sendRawPacket(uint8_t* data, size_t len);

        bool sendPacket(const lib::packet::A3MESHPacket_t& packet);

        bool startReceive();

        void clearDioActions();

        void setPacketReceivedAction();

        bool checkPacketReceived();

        int readData(uint8_t* buffer, size_t len);

        void handleInterrupt();

    private:
        SX1262 *radio;

        lora::A3MESHConfig config;

        static volatile bool receivedFlag;
    };
} // namespace lib::lora