#pragma once
#include "LoRaDriver.h"
#include "BLEDriver.h"
#include "A3MESHPacket.h"

namespace lib::mesh
{
    class A3MESHManager
    {
    public:
        A3MESHManager();

        bool begin();
        void attachBLE(lib::ble::BLEDriver *b);
        void attachLoRa(lib::lora::LoRaDriver *l);
        uint16_t getNodeID();
        uint16_t getNextMsgID();
        
        void sendDiscovery();
    private:

        uint16_t _node_id;
        uint16_t _msg_counter = 0;
        SemaphoreHandle_t _mutex;
        static void sendDiscoveryTask(void *pvParameters);
        uint32_t generateNodeID();
        
    };
}