#pragma once

#include <RadioLib.h>
#include "LoRaConfig.h"
#include "A3MESHPacket.h"

namespace lib::lora
{
    typedef struct
    {
        uint8_t data[RX_BUFFER_SIZE];
        size_t len;
    } RxMessage_t;

    class LoRaDriver
    {
    public:
        LoRaDriver();

        TaskHandle_t processPacketTaskHandle;

        bool begin(const lora::A3MESHConfig &config);

        bool sendRawPacket(uint8_t *data, size_t len);

        bool sendPacket(const lib::packet::A3MESHPacket_t &packet);

        bool startReceive();

        void clearDioActions();

        void setPacketReceivedAction();

        bool checkPacketReceived();

        int readData(uint8_t *buffer, size_t len);

        bool startTasks();

    private:
        SX1262 *radio;

        lora::A3MESHConfig config;

        static volatile bool receivedFlag;

        QueueHandle_t rxQueue;
        QueueHandle_t txQueue;

        static void rxLoRaPacketTask(void *pvParameters);
        static void txLoRaPacketTask(void *pvParameters);
        static void processLoRaPacketTask(void *pvParameters);

    };
} // namespace lib::lora