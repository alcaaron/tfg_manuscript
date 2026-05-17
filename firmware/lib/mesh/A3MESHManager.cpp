#include "A3MESHManager.h"
#include <freertos/queue.h>
#include <freertos/task.h>

namespace lib::mesh
{

    A3MESHManager::A3MESHManager()
    {
        _mutex = xSemaphoreCreateMutex();
        if (!_mutex)
        {
            Serial.println("Mutex creation failed!");
            while (1)
                ;
        }
        _node_id = generateNodeID();
        Serial.printf("Node ID: %u\n", _node_id);
        _routing_protocol = 0;
    }

    uint16_t A3MESHManager::getNodeID()
    {
        return _node_id;
    }

    uint16_t A3MESHManager::getNextMsgID()
    {

        uint16_t id;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        id = _msg_counter++;
        xSemaphoreGive(_mutex);

        return id;
    }

    uint32_t A3MESHManager::generateNodeID()
    {

        uint64_t mac = ESP.getEfuseMac();
        return (uint32_t)((mac >> 16) ^ mac);
    }

    bool A3MESHManager::shouldForward(lib::packet::A3MESHPacket_t *pkt)
    {
        // Check TTL is greater than 0
        if (pkt->ttl == 0) { return false; }

        if (_routing_protocol == 0)
        {
            // Flooding
            return true;
        }
        else if (_routing_protocol == 1)
        {
            // Gossip with a P value
            return (rand() % 100) < 15;
        }
    }
}