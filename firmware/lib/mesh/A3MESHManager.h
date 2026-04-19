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
        
        // LoRa
        void sendDiscovery();

        // BLE


    private:

        uint16_t _node_id;
        uint16_t _msg_counter = 0;
        SemaphoreHandle_t _mutex;


        lib::ble::BLEDriver *ble = nullptr;
        lib::lora::LoRaDriver *lora = nullptr;

        static void sendDiscoveryTask(void *pvParameters);
        static void BLE2LoRaTask(void *pvParameters);
        static void LoRa2BLETask(void *pvParameters);

        uint32_t generateNodeID();
        uint16_t getNextMsgID();
    };
}