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
            Serial.println("[A3MESH] Mutex creation failed!");
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

    bool A3MESHManager::isDuplicate(const lib::packet::A3MESHPacket_t &pkt)
    {
        uint32_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (int i = 0; i < DUP_CACHE_SIZE; i++) {

            if (seenCache[i].msgID == pkt.pkt_id &&
                seenCache[i].src == pkt.src &&
                (now - seenCache[i].timestamp < DUP_TIMEOUT)) {

                xSemaphoreGive(_mutex);
                return true;
            }
        }

        seenCache[seenIndex].msgID = pkt.pkt_id;
        seenCache[seenIndex].src = pkt.src;
        seenCache[seenIndex].timestamp = now;

        seenIndex = (seenIndex + 1) % DUP_CACHE_SIZE;

        xSemaphoreGive(_mutex);

        return false;
    }

    void A3MESHManager::updateNeighbor(const Neighbor &neighbor)
    {
        Serial.printf("[NEIGHBOR] Node ID: %u, RSSI: %d\n", neighbor.nodeID, neighbor.rssi);

        TickType_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (uint8_t i = 0; i < neighborCount; i++)
        {

            if (neighbors[i].nodeID == neighbor.nodeID)
            {
                Serial.printf("[NEIGHBOR] Updating node %u\n", neighbor.nodeID);
                neighbors[i].rssi = neighbor.rssi;
                neighbors[i].lastSeen = neighbor.lastSeen;
                xSemaphoreGive(_mutex);
                return;
            }
        }

        if (neighborCount < MAX_NEIGHBORS)
        {
            Serial.printf("[NEIGHBOR] Adding node %u\n", neighbor.nodeID);
            neighbors[neighborCount++] = neighbor;
        }

        Serial.printf("[NEIGHBOR] Total neighbors: %d\n", neighborCount);
        xSemaphoreGive(_mutex);
    }

    void A3MESHManager::removeTimedOutNeighbors()
    {
        TickType_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (uint8_t i = 0; i < neighborCount; i++)
        {
            if (now - neighbors[i].lastSeen > NODE_TIMEOUT)
            {
                Serial.printf(
                    "[NEIGHBOR] Removing node %u due to timeout\n",
                    neighbors[i].nodeID);

                for (uint8_t j = i; j < neighborCount - 1; j++)
                {
                    neighbors[j] = neighbors[j + 1];
                }

                neighborCount--;
                i--;
            }
        }

        xSemaphoreGive(_mutex);
    }

    uint8_t A3MESHManager::getNeighborCount()
    {

        uint8_t count;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        count = neighborCount;
        xSemaphoreGive(_mutex);

        return count;
    }

    uint32_t A3MESHManager::generateNodeID()
    {

        uint64_t mac = ESP.getEfuseMac();
        return (uint32_t)((mac >> 16) ^ mac);
    }

    uint16_t A3MESHManager::GetBestNeighborID()
    {
        uint16_t best_id = 0xFFFF;
        int16_t best_rssi = -127;

        Serial.println("[MESH] Selecting best neighbor for forwarding...");

        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (neighborCount == 0)
        {
            Serial.println("[MESH] No neighbors available, using broadcast address");
            xSemaphoreGive(_mutex);
            return best_id; // No neighbors, return broadcast
        }
        else
        {
            best_id = neighbors[0].nodeID;
            best_rssi = neighbors[0].rssi;
        }

        for (uint8_t i = 0; i < neighborCount; i++)
        {
            if (neighbors[i].rssi > best_rssi)
            {
                best_rssi = neighbors[i].rssi;
                best_id = neighbors[i].nodeID;
            }
        }

        xSemaphoreGive(_mutex);

        Serial.printf("[MESH] Best neighbor selected: %u with RSSI %d\n", best_id, best_rssi);
        return best_id;
    }

    bool A3MESHManager::IsNeighbor(uint16_t nodeID)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (neighborCount == 0)
        {
            Serial.println("[MESH] No neighbors available");
            xSemaphoreGive(_mutex);
            return false; // No neighbors, return false
        }

        for (uint8_t i = 0; i < neighborCount; i++)
        {
            if (neighbors[i].nodeID == nodeID)
            {
                xSemaphoreGive(_mutex);
                return true; // Neighbor found, return true
            }
        }

        xSemaphoreGive(_mutex);

        return false; // Neighbor not found, return false
    }

    bool A3MESHManager::shouldForward(const lib::packet::A3MESHPacket_t &pkt)
    {
        // Check TTL is greater than 0
        if (pkt.ttl == 0) return false;

        // Only forward if I'm the selected neighbor or if it's a broadcast
        if (pkt.neighbor_id != _node_id && pkt.neighbor_id != 0xFFFF) return false;

        // Is duplicate?
        if (isDuplicate(pkt)) return false;

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
        else
        {
            // Default to no forwarding
            return false;
        }
    }
}