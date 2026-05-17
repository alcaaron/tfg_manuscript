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
        uint16_t getNodeID();
        uint16_t getNextMsgID();
        bool shouldForward(lib::packet::A3MESHPacket_t *pkt);
    private:

        uint16_t _node_id;
        uint16_t _msg_counter = 0;
        SemaphoreHandle_t _mutex;
        uint32_t generateNodeID();
        uint8_t _routing_protocol;
        
    };
}