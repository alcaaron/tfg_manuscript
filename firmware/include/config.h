#ifndef CONFIG_H
#define CONFIG_H

// ==============
// === MACROS ===
// ==============

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define BLE_MSG_STARTS_WITH(str, prefix) (strncmp(str, prefix, strlen(prefix)) == 0)

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

#define MAX_NEIGHBORS               10
#define NODE_TIMEOUT                60000           // Time in ms after which a neighbor is considered lost
#define MAX_TTL                     0x05
#define A3MESH_MAX_PAYLOAD          241             // LORA_MAX_PACKET_SIZE - (size of header + fixed fields in packet structure)
#define DUP_CACHE_SIZE              50              // Number of recently seen packets to keep in cache for duplicate detection
#define DUP_TIMEOUT                 30000           // Time in ms to consider a packet as duplicate


#endif // CONFIG_H