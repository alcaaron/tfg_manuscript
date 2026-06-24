#ifndef CONFIG_H
#define CONFIG_H

// ==============
// === MACROS ===
// ==============

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define BLE_MSG_STARTS_WITH(str, prefix) (strncmp(str, prefix, sizeof(prefix) - 1) == 0)

// ============================
// === GENERAL CONFIGURATION ===
// ============================

#define BAUD_RATE                   115200

// =============================
// === A3MESH APP PARAMETERS ===
// =============================

#define A3M_VERSION                 1
#define A3M_TAG                     "A3MESH"


// =============================
// === RF LORA CONFIGURATION ===
// =============================

#define EU868_FREQUENCY             868000000L       // LoRa frequency in MHz. For example, 868.0 for EU868, 915.0 for US915, etc.
#define BANDWIDTH_125_KHZ           125E3           // LoRa bandwidth in kHz.
#define A3MESH_PREAMBLE_LENGTH      0x08               // Length of LoRa transmission preamble in symbols.
#define A3MESH_SF                   0x08               // LoRa spreading factor. Allowed values range from 6 to 12.
#define A3MESH_CR                   0x01               // 5: 4/5, 6: 4/6, 7: 4/7, 8: 4/8
#define A3MESH_SYNC_WORD            0x12            // LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
#define A3MESH_POWER                0x14              // Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
#define LORA_MAX_PACKET_SIZE        256


// ======================
// === LORA CHIP PINS ===
// ======================

#define SCK_GPIO                    0x05
#define MISO_GPIO                   0x0F
#define MOSI_GPIO                   0x19
#define NSS_GPIO                    0x12
#define RST_GPIO                    0x0E
#define DIO0_GPIO                   0x1A


// ======================================
// === LORA TASKS & QUEUES PARAMETERS ===
// ======================================

#define RX_LORA_QUEUE_SIZE          20
#define TX_LORA_QUEUE_SIZE          20

#define POLLING_INTERVAL_MS         500


// =========================
// === BLE CONFIGURATION ===
// =========================

#define BLE_DEVICE_NAME             "A3MESH_Node"
#define SERVICE_UUID                "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_RX           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_TX           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_BUFFER_SIZE             256


// =====================================
// === BLE TASKS & QUEUES PARAMETERS ===
// =====================================

#define BLE_RX_QUEUE_SIZE           10
#define BLE_TX_QUEUE_SIZE           10

#define BLE_LIFE_BIT_INTERVAL_MS   120000

// ===============================
// === A3MESH LOGIC PARAMETERS ===
// ===============================

#define DISCOVERY_INTERVAL_MS       30000
#define FNV_OFFSET_BASIS            2166136261U
#define FNV_PRIME                   16777619U
#define MAX_NEIGHBORS               10
#define NODE_TIMEOUT                180000           // Time in ms after which a neighbor is considered lost
#define MAX_TTL                     0x05
#define MIN_TTL                     0x01
#define A3MESH_MAX_PAYLOAD          233             // LORA_MAX_PACKET_SIZE - (HEADER_SIZE + FIXED_FIELDS_SIZE); reduced by 2 for the crc field
#define DUP_CACHE_SIZE              50              // Number of recently seen packets to keep in cache for duplicate detection
#define DUP_TIMEOUT                 30000           // Time in ms to consider a packet as duplicate
#define MAX_PENDING_FORWARDS        8               // Max simultaneous forwards awaiting passive-ACK confirmation
#define BROADCAST_NODE_ID            0xFFFFFFFF      // Special node ID representing broadcast (used in pkt.dst)
#define DIRECT_UNICAST_FLAG             0x01            // Packet flag indicating a direct unicast (as opposed to a broadcast or multi-hop message)
#define BROADCAST_FLAG                  0x00            // Packet flag indicating a broadcast message


// ================================
// === SMART GOSSIP PARAMETERS ===
// ================================
// p = max(GOSSIP_P_FLOOR, f_rssi * f_density * f_dupe)
//
//   f_rssi    = 1 / (1 + exp(GOSSIP_ALPHA * (rssi - GOSSIP_RSSI_REF)))   logistic sigmoid
//   f_density = 1 / (1 + GOSSIP_BETA * n_neighbors)                      rational decay
//   f_dupe    = exp(-GOSSIP_GAMMA * (copies - 1))                         exponential decay
//

#define GOSSIP_ALPHA          0.145f    // RSSI sigmoid slope (dBm⁻¹)
#define GOSSIP_RSSI_REF      -103.8f   // RSSI sigmoid inflection point (dBm)
#define GOSSIP_BETA           0.146f   // Density decay coefficient
#define GOSSIP_GAMMA          0.311f   // Exponential penalty for duplicate packets
#define GOSSIP_P_FLOOR        0.111f   // Minimum forwarding probability
#define GOSSIP_MAX_COPIES     3        // Discard packet after receiving more than N copies (anti-loop)


// =======================================
// === LORA DUTY CYCLE CONFIGURATION ===
// =======================================
// EU868 regulations: max 1% duty cycle over a 1-hour window.

#define LORA_DUTY_CYCLE_WINDOW_MS   3600000UL  // Sliding window duration (1 hour)
#define LORA_DUTY_CYCLE_MAX_MS      36000UL    // Max airtime per window (1% of 3600 s = 36 s)
#define LORA_AIRTIME_RECORDS        20         // Ring buffer depth for recent TX records


// ==============================
// === FIRMWARE VERSION ===
// ==============================

#define A3M_FW_VERSION               "1.0.0"


// ===============================
// === BATTERY CONFIGURATION ===
// ===============================

#define BATTERY_VOLTAGE_PIN              A0        // ADC pin connected to battery voltage divider
#define BATTERY_THRESHOLD                3.0f      // Voltage threshold for low battery warning
#define BATTERY_MEASUREMENT_INTERVAL_MS  1000      // Interval for battery voltage measurements
#define BATTERY_ADC_FACTOR               2.0f      // Multiply ADC voltage by this (accounts for voltage divider)
#define BATTERY_MAX_V                    4.2f      // Nominal full LiPo voltage
#define BATTERY_MIN_V                    3.0f      // Nominal empty LiPo voltage


// ==============================
// === DEBUG CONFIGURATION ===
// ==============================

#define A3MESH_DEBUG 1

#if A3MESH_DEBUG
  #define MESH_LOG(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define MESH_LOG(fmt, ...) do {} while(0)
#endif

#endif // CONFIG_H