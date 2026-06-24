#include "A3MESHManager.hh"
#include <freertos/queue.h>
#include <freertos/task.h>
#include <math.h>

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
    }

    uint32_t A3MESHManager::get_node_id()
    {
        return _node_id;
    }

    uint16_t A3MESHManager::get_next_pkt_id()
    {
        return _msg_counter.fetch_add(1, std::memory_order_relaxed);
    }

    uint8_t A3MESHManager::get_copies(const lib::packet::A3MESH_packet_t &pkt)
    {
        uint32_t now = millis();

        xSemaphoreTake(_mutex, portMAX_DELAY);

        int evict_idx = -1;

        for (int i = 0; i < DUP_CACHE_SIZE; i++) {
            if (seen_cache[i].msg_id == pkt.pkt_id && seen_cache[i].src == pkt.src) {
                if (now - seen_cache[i].timestamp < DUP_TIMEOUT) {
                    seen_cache[i].copies++;
                    uint8_t c = seen_cache[i].copies;
                    xSemaphoreGive(_mutex);
                    MESH_LOG("[MESH] Copy #%u: pkt_id=%u src=%08x\r\n", c, pkt.pkt_id, pkt.src);
                    return c;
                }
                // Same key but expired: refresh in place
                evict_idx = i;
                break;
            }
            if (evict_idx == -1 && (now - seen_cache[i].timestamp >= DUP_TIMEOUT))
                evict_idx = i;
        }

        int target = (evict_idx >= 0) ? evict_idx : (int)seen_index;
        seen_cache[target].msg_id    = pkt.pkt_id;
        seen_cache[target].src       = pkt.src;
        seen_cache[target].timestamp = now;
        seen_cache[target].copies    = 1;

        if (evict_idx < 0)
            seen_index = (seen_index + 1) % DUP_CACHE_SIZE;

        xSemaphoreGive(_mutex);
        return 1;
    }

    bool A3MESHManager::update_neighbor(const Neighbor &neighbor)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        for (uint8_t i = 0; i < neighbor_count; i++)
        {
            if (neighbors[i].node_id == neighbor.node_id)
            {
                neighbors[i].rssi = neighbor.rssi;
                neighbors[i].last_seen = neighbor.last_seen;
                xSemaphoreGive(_mutex);
                return false;
            }
        }

        if (neighbor_count < MAX_NEIGHBORS)
        {
            Serial.printf("[NEIGHBOR] Adding node %hx; Total=%d\n", neighbor.node_id, neighbor_count+1);
            neighbors[neighbor_count++] = neighbor;
        }

        xSemaphoreGive(_mutex);
        return true;
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

                neighbors[i] = neighbors[--neighbor_count];
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

    bool A3MESHManager::is_neighbor(uint32_t node_id)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (neighbor_count == 0)
        {
            MESH_LOG("[MESH] No neighbors available\r\n");
            xSemaphoreGive(_mutex);
            return false;
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
        xSemaphoreTake(_mutex, portMAX_DELAY);
        memcpy(neighbors_list, neighbors, sizeof(Neighbor) * neighbor_count);
        xSemaphoreGive(_mutex);
    }

    void A3MESHManager::register_pending_forward(uint16_t pkt_id, uint32_t src)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (int i = 0; i < MAX_PENDING_FORWARDS; i++) {
            if (!_pending_forwards[i].active) {
                _pending_forwards[i].pkt_id    = pkt_id;
                _pending_forwards[i].src       = src;
                _pending_forwards[i].active    = true;
                _pending_forwards[i].cancelled = false;
                xSemaphoreGive(_mutex);
                return;
            }
        }
        MESH_LOG("[MESH] Pending forward table full, cannot register pkt_id=%u\r\n", pkt_id);
        xSemaphoreGive(_mutex);
    }

    void A3MESHManager::cancel_pending_forward(uint16_t pkt_id, uint32_t src)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (int i = 0; i < MAX_PENDING_FORWARDS; i++) {
            if (_pending_forwards[i].active      &&
                _pending_forwards[i].pkt_id == pkt_id &&
                _pending_forwards[i].src    == src) {
                _pending_forwards[i].cancelled = true;
                MESH_LOG("[MESH] Forward suppressed (passive ACK): pkt_id=%u, src=%08x\r\n", pkt_id, src);
                break;
            }
        }
        xSemaphoreGive(_mutex);
    }


    bool A3MESHManager::consume_pending_forward(uint16_t pkt_id, uint32_t src)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (int i = 0; i < MAX_PENDING_FORWARDS; i++) {
            if (_pending_forwards[i].active      &&
                _pending_forwards[i].pkt_id == pkt_id &&
                _pending_forwards[i].src    == src) {
                bool transmit = !_pending_forwards[i].cancelled;
                _pending_forwards[i].active = false;
                xSemaphoreGive(_mutex);
                return transmit;
            }
        }
        xSemaphoreGive(_mutex);
        return true;  // entry not found: transmit by default
    }

    uint8_t A3MESHManager::gossip_forward_probability(uint8_t n_neighbors, int16_t rssi, uint8_t copies)
    {
        float f_rssi    = 1.0f / (1.0f + expf(GOSSIP_ALPHA * ((float)rssi - GOSSIP_RSSI_REF)));
        float f_density = 1.0f / (1.0f + GOSSIP_BETA  * (float)n_neighbors);
        float f_dupe    = expf(-GOSSIP_GAMMA * (float)(copies - 1));

        float p = f_rssi * f_density * f_dupe;
        if (p < GOSSIP_P_FLOOR) p = GOSSIP_P_FLOOR;

        return (uint8_t)(p * 100.0f + 0.5f);
    }

    bool A3MESHManager::should_forward(const lib::packet::A3MESH_packet_t &pkt, int16_t rssi, uint8_t copies)
    {
        if (pkt.ttl == 0)
        {
            MESH_LOG("[MESH] Drop: TTL is 0\r\n");
            return false;
        }

        if (pkt.flags & DIRECT_UNICAST_FLAG)
        {
            MESH_LOG("[MESH] Drop: direct message flag set\r\n");
            return false;
        }

        uint8_t n     = get_neighbor_count();
        uint8_t threshold = gossip_forward_probability(n, rssi, copies);
        // esp_random() is a thread-safe hardware TRNG
        bool forward = (esp_random() % 100) < threshold;

        MESH_LOG("[MESH] SmartGossip: n=%u rssi=%d copies=%u | P=%u%% → %s\r\n", 
            n, rssi, copies, threshold, forward ? "fwd" : "drop");

        return forward;
    }
}