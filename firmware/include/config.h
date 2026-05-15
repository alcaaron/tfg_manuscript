#ifndef CONFIG_H
#define CONFIG_H

#define A3M_VERSION 1
#define A3M_TAG "A3MESH"

#define EU868_FREQUENCY 868.0       // LoRa frequency in MHz. For example, 868.0 for EU868, 915.0 for US915, etc.
#define BANDWIDTH_125_KHZ 125.0     // LoRa bandwidth in kHz.
#define A3MESH_PREAMBLE_LENGTH 8    // Length of LoRa transmission preamble in symbols.
#define A3MESH_SF 8                // LoRa spreading factor. Allowed values range from 6 to 12.
#define A3MESH_CR 5                 // 5: 4/5, 6: 4/6, 7: 4/7, 8: 4/8
#define A3MESH_SYNC_WORD 0x12       // LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
#define A3MESH_POWER 14             // Transmission output power in dBm. Allowed values range from 2 to 17 dBm.

#define A3MESH_MAX_PACKET_SIZE 100  // Maximum size of an A3MESH packet in bytes.

#define SCK_GPIO 5
#define MISO_GPIO 15
#define MOSI_GPIO 25
#define NSS_GPIO 18
#define RST_GPIO 14
#define DIO0_GPIO 26

#define RX_LORA_QUEUE_SIZE 10
#define POLLING_INTERVAL_MS 500
#define RX_BUFFER_SIZE 100
#define TX_BUFFER_SIZE 100
#define TX_LORA_QUEUE_SIZE 10
#define A3MESH_MAX_PAYLOAD 220
#define MAX_TTL 5

#define BLE_DEVICE_NAME "A3MESH Node"
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_LIFE_BIT_INTERVAL_MS 30000
#define BLE_RX_QUEUE_SIZE 10
#define BLE_TX_QUEUE_SIZE 10
#define BLE_BUFFER_SIZE 256

#endif // CONFIG_H