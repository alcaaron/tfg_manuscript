#pragma once
#include "LoRaDriver.h"
#include "BLEDriver.h"
#include "A3MESHPacket.h"

namespace lib::mesh
{
    typedef struct {
        uint16_t nodeID;
        int16_t rssi;
        uint32_t lastSeen;
    } Neighbor;

    typedef struct {
        uint16_t msgID;
        uint16_t src;
        uint32_t timestamp;
    } SeenMessage;

    class A3MESHManager
    {
    public:
        A3MESHManager();

        bool begin();
        uint16_t getNodeID();
        uint16_t getNextMsgID();
        
        void sendDiscovery();
        bool shouldForward(const lib::packet::A3MESHPacket_t &pkt);
        void updateNeighbor(const Neighbor &neighbor);
        bool isDuplicate(const lib::packet::A3MESHPacket_t &pkt);
        uint8_t getNeighborCount();
        void removeTimedOutNeighbors();
        uint16_t GetBestNeighborID();
        bool IsNeighbor(uint16_t nodeID);


    private:

        uint16_t _node_id;
        uint16_t _msg_counter = 0;
        SemaphoreHandle_t _mutex;
        uint32_t generateNodeID();
        uint8_t _routing_protocol;
        Neighbor neighbors[MAX_NEIGHBORS];
        uint8_t neighborCount = 0;

        SeenMessage seenCache[DUP_CACHE_SIZE];
        uint8_t seenIndex = 0;



        
    };
}