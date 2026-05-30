#pragma once
#include "LoRaDriver.hh"
#include "BLEDriver.hh"
#include "A3MESHPacket.h"

namespace lib::mesh
{
    typedef struct {
        uint32_t node_id;
        int16_t rssi;
        uint32_t last_seen;
    } Neighbor;

    typedef struct {
        uint16_t msg_id;
        uint32_t src;
        uint32_t timestamp;
    } SeenMessage;

    class A3MESHManager
    {
    public:
        A3MESHManager();

        bool begin();
        uint32_t get_node_id();
        uint16_t get_next_pkt_id();
        
        void send_discovery();
        bool should_forward(const lib::packet::A3MESH_packet_t &pkt);
        void update_neighbor(const Neighbor &neighbor);
        bool is_duplicate(const lib::packet::A3MESH_packet_t &pkt);
        uint8_t get_neighbor_count();
        void rm_tout_neigbhors();
        uint32_t get_best_neighbor_id();
        bool is_neighbor(uint32_t node_id);
        void get_neighbors(Neighbor *neighbors_list);


    private:

        uint32_t _node_id;
        uint16_t _msg_counter = 0;
        SemaphoreHandle_t _mutex;
        uint32_t generate_node_id();
        uint8_t _routing_protocol;
        Neighbor neighbors[MAX_NEIGHBORS];
        uint8_t neighbor_count = 0;

        SeenMessage seen_cache[DUP_CACHE_SIZE];
        uint8_t seen_index = 0;



        
    };
}