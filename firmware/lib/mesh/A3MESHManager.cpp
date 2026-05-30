#include "A3MESHManager.hh"
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
        _node_id = generate_node_id();
        Serial.printf("Node ID: %u\n", _node_id);
        _routing_protocol = 1;
    }

    uint32_t A3MESHManager::get_node_id()
    {
        return _node_id;
    }

    uint16_t A3MESHManager::get_next_pkt_id()
    {

        uint16_t id;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        id = _msg_counter++;
        xSemaphoreGive(_mutex);

        return id;
    }

    bool A3MESHManager::is_duplicate(const lib::packet::A3MESH_packet_t &pkt)
    {
        uint32_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (int i = 0; i < DUP_CACHE_SIZE; i++) {

            if (seen_cache[i].msg_id == pkt.pkt_id &&
                seen_cache[i].src == pkt.src &&
                (now - seen_cache[i].timestamp < DUP_TIMEOUT)) {

                xSemaphoreGive(_mutex);
                return true;
            }
        }

        seen_cache[seen_index].msg_id = pkt.pkt_id;
        seen_cache[seen_index].src = pkt.src;
        seen_cache[seen_index].timestamp = now;

        seen_index = (seen_index + 1) % DUP_CACHE_SIZE;

        xSemaphoreGive(_mutex);

        return false;
    }

    void A3MESHManager::update_neighbor(const Neighbor &neighbor)
    {
        //Serial.printf("[NEIGHBOR] Node ID: %hx, RSSI: %d\n", neighbor.node_id, neighbor.rssi);

        TickType_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (uint8_t i = 0; i < neighbor_count; i++)
        {

            if (neighbors[i].node_id == neighbor.node_id)
            {
                //Serial.printf("[NEIGHBOR] Updating node %hx\n", neighbor.node_id);
                neighbors[i].rssi = neighbor.rssi;
                neighbors[i].last_seen = neighbor.last_seen;
                xSemaphoreGive(_mutex);
                return;
            }
        }

        if (neighbor_count < MAX_NEIGHBORS)
        {
            Serial.printf("[NEIGHBOR] Adding node %hx; Total=%d\n", neighbor.node_id, neighbor_count+1);
            neighbors[neighbor_count++] = neighbor;
        }

        xSemaphoreGive(_mutex);
    }

    void A3MESHManager::rm_tout_neigbhors()
    {
        TickType_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (uint8_t i = 0; i < neighbor_count; i++)
        {
            if (now - neighbors[i].last_seen > NODE_TIMEOUT)
            {
                Serial.printf(
                    "[NEIGHBOR] Removing node %hx due to timeout\n",
                    neighbors[i].node_id);

                for (uint8_t j = i; j < neighbor_count - 1; j++)
                {
                    neighbors[j] = neighbors[j + 1];
                }

                neighbor_count--;
                i--;
            }
        }

        xSemaphoreGive(_mutex);
    }

    uint8_t A3MESHManager::get_neighbor_count()
    {

        uint8_t count;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        count = neighbor_count;
        xSemaphoreGive(_mutex);

        return count;
    }

    uint32_t A3MESHManager::generate_node_id()
    {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BT);

        uint32_t hash = FNV_OFFSET_BASIS;

        for (int i = 0; i < 6; i++)
        {
            hash ^= mac[i];
            hash *= FNV_PRIME;
        }

        return hash;
    }

    uint32_t A3MESHManager::get_best_neighbor_id()
    {
        uint32_t best_id = 0xFFFF;
        int16_t best_rssi = -127;

        Serial.println("[MESH] Selecting best neighbor for forwarding...");

        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (neighbor_count == 0)
        {
            Serial.println("[MESH] No neighbors available, using broadcast address");
            xSemaphoreGive(_mutex);
            return best_id; // No neighbors, return broadcast
        }
        else
        {
            best_id = neighbors[0].node_id;
            best_rssi = neighbors[0].rssi;
        }

        for (uint8_t i = 0; i < neighbor_count; i++)
        {
            if (neighbors[i].rssi > best_rssi)
            {
                best_rssi = neighbors[i].rssi;
                best_id = neighbors[i].node_id;
            }
        }

        xSemaphoreGive(_mutex);

        Serial.printf("[MESH] Best neighbor selected: %hx with RSSI %d\n", best_id, best_rssi);
        return best_id;
    }

    bool A3MESHManager::is_neighbor(uint32_t node_id)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (neighbor_count == 0)
        {
            Serial.println("[MESH] No neighbors available");
            xSemaphoreGive(_mutex);
            return false; // No neighbors, return false
        }

        for (uint8_t i = 0; i < neighbor_count; i++)
        {
            if (neighbors[i].node_id == node_id)
            {
                xSemaphoreGive(_mutex);
                return true; // Neighbor found, return true
            }
        }

        xSemaphoreGive(_mutex);

        return false; // Neighbor not found, return false
    }

    void A3MESHManager::get_neighbors(Neighbor *neighbors_list)
    {
        memcpy(neighbors_list, neighbors, sizeof(Neighbor) * neighbor_count);
    }

    bool A3MESHManager::should_forward(const lib::packet::A3MESH_packet_t &pkt)
    {
        // Check TTL is greater than 0
        if (pkt.ttl == 0) 
        {
            Serial.println("[MESH] Not forwarding packet because TTL is 0");
            return false;
        }

        // Is a Direct message?
        if (pkt.flags & 0x01) 
        {
            Serial.println("[MESH] Not forwarding packet because it is a direct message");
            return false;
        }

        if (_routing_protocol == 0)
        {
            // Flooding
            return true;
        }
        else if (_routing_protocol == 1)
        {
            // Gossip with a P value
            Serial.println("[MESH] Forwarding packet with gossip routing (P=0.9)");
            uint8_t will_be_forwarded = (rand() % 100) < 90; // 90% chance to forward
            Serial.printf("[MESH] Gossip routing: will be forwarded: %s\r\n", (will_be_forwarded) ? "yes" : "no");
            return will_be_forwarded; // 90% chance to forward
        }
        else
        {
            // Default to no forwarding
            return false;
        }
    }
}