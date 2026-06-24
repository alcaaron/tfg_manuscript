#pragma once
#include "LoRaDriver.hh"
#include "BLEDriver.hh"
#include "A3MESHPacket.h"
#include <atomic>

namespace lib::mesh
{
    /*!
     * \brief Represents a single known neighbour in the mesh.
     */
    typedef struct {
        uint32_t node_id;   /*!< Unique identifier of the neighbour node (FNV-1a of BT MAC). */
        int16_t  rssi;      /*!< Last measured RSSI in dBm from this neighbour. */
        uint32_t last_seen; /*!< millis() timestamp of the last received frame from this node. */
    } Neighbor;

    /*!
     * \brief Entry in the duplicate-detection cache.
     *
     * Tracks how many copies of a given (pkt_id, src) pair have been
     * received within the DUP_TIMEOUT window.
     */
    typedef struct {
        uint16_t msg_id;    /*!< Packet identifier (A3MESH_packet_t::pkt_id). */
        uint32_t src;       /*!< Source node ID (A3MESH_packet_t::src). */
        uint32_t timestamp; /*!< millis() when this entry was first inserted. */
        uint8_t  copies;    /*!< Number of times this (msg_id, src) pair has been received. */
    } SeenMessage;

    /*!
     * \brief Entry in the passive-ACK pending-forward table.
     *
     * A forward is registered before its jitter delay starts.  If a
     * neighbour is overheard relaying the same packet during the window,
     * the entry is cancelled and the local TX is suppressed.
     */
    typedef struct {
        uint16_t pkt_id;    /*!< Mesh packet ID of the queued forward. */
        uint32_t src;       /*!< Source node ID of the queued forward. */
        bool     active;    /*!< True while this slot holds a valid pending forward. */
        bool     cancelled; /*!< True if a neighbour was overheard forwarding this packet. */
    } PendingForward;

    /*!
     * \brief Central coordinator for the A3MESH mesh layer.
     *
     * Manages the neighbour table, duplicate-detection cache, packet-counter,
     * passive-ACK suppression table, and the Smart Gossip forwarding decision.
     * All methods that access shared state are protected by an internal mutex
     * (or by lock-free atomics for simple scalar fields).
     */
    class A3MESHManager
    {
    public:
        /*!
         * \brief Constructs the manager and derives the node ID from the BT MAC address.
         *
         * Creates the internal FreeRTOS mutex.  Halts the system if mutex
         * creation fails (out-of-heap condition).
         */
        A3MESHManager();

        bool begin();

        /*!
         * \brief Returns the locally derived node identifier.
         *
         * The ID is a 32-bit FNV-1a hash of the ESP32 Bluetooth MAC address,
         * computed once at construction time.
         *
         * \retval uint32_t  This node's unique identifier.
         */
        uint32_t get_node_id();

        /*!
         * \brief Returns the next monotonically incrementing packet ID and
         *        advances the counter.
         *
         * Lock-free; uses an atomic fetch-add with relaxed ordering.
         *
         * \retval uint16_t  Unique packet identifier for the next outgoing frame.
         */
        uint16_t get_next_pkt_id();

        void send_discovery();

        /*!
         * \brief Decides whether an incoming packet should be forwarded.
         *
         * Applies the Smart Gossip algorithm: probability is a function of
         * RSSI (sigmoid), neighbour density (rational decay), and duplicate
         * count (exponential decay).  Returns false unconditionally when TTL
         * is 0 or the direct-message flag is set.
         *
         * \param [IN] pkt     Packet under evaluation.
         * \param [IN] rssi    RSSI of the received frame in dBm.
         * \param [IN] copies  Number of times this (pkt_id, src) has been seen
         *                     (1 = first receipt, 2+ = duplicate).
         *
         * \retval true   The packet should be forwarded.
         * \retval false  The packet should be dropped.
         */
        bool should_forward(const lib::packet::A3MESH_packet_t &pkt, int16_t rssi, uint8_t copies);

        /*!
         * \brief Inserts or refreshes a neighbour entry in the neighbour table.
         *
         * If the node is already known, its RSSI and last_seen timestamp are
         * updated.  If the table is not full and the node is new, it is appended.
         *
         * \param [IN] neighbor  Neighbour data to insert or refresh.
         *
         * \retval true   The neighbour was newly added.
         * \retval false  An existing entry was updated (not a new neighbour).
         */
        bool update_neighbor(const Neighbor &neighbor);

        /*!
         * \brief Records a reception of a (pkt_id, src) pair and returns the
         *        running copy count within the DUP_TIMEOUT window.
         *
         * On the first reception, a new cache slot is allocated (LRU eviction
         * when full) and 1 is returned.  On subsequent receptions within the
         * window, the copy counter is incremented and returned.
         *
         * \param [IN] pkt  Packet whose (pkt_id, src) is to be recorded.
         *
         * \retval uint8_t  Number of times this packet has now been received
         *                  (1 = first time, 2+ = duplicate).
         */
        uint8_t get_copies(const lib::packet::A3MESH_packet_t &pkt);

        /*!
         * \brief Returns the current number of entries in the neighbour table.
         *
         * \retval uint8_t  Neighbour count in [0, MAX_NEIGHBORS].
         */
        uint8_t get_neighbor_count();

        /*!
         * \brief Evicts all neighbour entries whose last_seen timestamp is
         *        older than NODE_TIMEOUT milliseconds.
         *
         * Uses a swap-last strategy (O(1) per removal) to avoid shifting.
         * Called periodically by vNeighborCheckTask.
         */
        void rm_tout_neigbhors();

        /*!
         * \brief Returns the node ID of the neighbour with the highest RSSI.
         *
         * Falls back to 0xFFFF (broadcast address) when the neighbour table
         * is empty.
         *
         * \retval uint32_t  Node ID of the best (strongest signal) neighbour,
         *                   or 0xFFFF if no neighbours are known.
         */
        uint32_t get_best_neighbor_id();

        /*!
         * \brief Checks whether a given node ID is in the neighbour table.
         *
         * \param [IN] node_id  Node ID to look up.
         *
         * \retval true   The node is a known neighbour.
         * \retval false  The node is not in the neighbour table.
         */
        bool is_neighbor(uint32_t node_id);

        /*!
         * \brief Copies the current neighbour table into a caller-supplied buffer.
         *
         * \param [OUT] neighbors_list  Buffer of at least MAX_NEIGHBORS Neighbor
         *                              entries.  Only the first get_neighbor_count()
         *                              entries are populated.
         */
        void get_neighbors(Neighbor *neighbors_list);

        /*!
         * \brief Registers a pending forward in the passive-ACK suppression table.
         *
         * Must be called before enqueueing the forward in xLoRaTxQueue so that
         * the slot exists if a cancellation arrives during the jitter delay.
         *
         * \param [IN] pkt_id  Packet ID of the frame to be forwarded.
         * \param [IN] src     Source node ID of the frame to be forwarded.
         */
        void register_pending_forward(uint16_t pkt_id, uint32_t src);

        /*!
         * \brief Marks a pending forward as cancelled (passive-ACK received).
         *
         * Called when a neighbour is overheard relaying the same packet during
         * the jitter delay window, suppressing our own redundant retransmission.
         *
         * \param [IN] pkt_id  Packet ID of the suppressed forward.
         * \param [IN] src     Source node ID of the suppressed forward.
         */
        void cancel_pending_forward(uint16_t pkt_id, uint32_t src);

        /*!
         * \brief Atomically reads and removes a pending forward entry.
         *
         * Called by vLoRaTxTask just before transmission to check whether the
         * forward was cancelled during the jitter window.
         *
         * \param [IN] pkt_id  Packet ID to look up.
         * \param [IN] src     Source node ID to look up.
         *
         * \retval true   The forward should proceed (not cancelled, or entry not found).
         * \retval false  The forward was cancelled by a passive ACK; TX must be suppressed.
         */
        bool consume_pending_forward(uint16_t pkt_id, uint32_t src);


    private:

        uint32_t _node_id;

        /*!< Monotonically incrementing packet counter; accessed lock-free via atomic. */
        std::atomic<uint16_t> _msg_counter{0};

        SemaphoreHandle_t _mutex;

        uint32_t generate_node_id();

        /*!
         * \brief Computes the Smart Gossip forwarding probability as an integer
         *        percentage [0, 100].
         *
         * p = max(p_floor, f_rssi * f_density * f_dupe)
         *
         * \param [IN] n_neighbors  Current number of known neighbours.
         * \param [IN] rssi         RSSI of the received frame in dBm.
         * \param [IN] copies       Number of times this packet has been received.
         *
         * \retval uint8_t  Forwarding probability in [0, 100] (percent).
         */
        static uint8_t gossip_forward_probability(uint8_t n_neighbors, int16_t rssi, uint8_t copies);

        Neighbor neighbors[MAX_NEIGHBORS];
        uint8_t neighbor_count = 0;

        SeenMessage seen_cache[DUP_CACHE_SIZE];
        uint8_t seen_index = 0;

        PendingForward _pending_forwards[MAX_PENDING_FORWARDS] = {};
    };
}
