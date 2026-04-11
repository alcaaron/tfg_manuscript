#ifndef CONFIG_H
#define CONFIG_H

#define A3M_VERSION "0.1.0"
#define A3M_TAG "A3MESH"

#define EU868_FREQUENCY 868.0       // LoRa frequency in MHz. For example, 868.0 for EU868, 915.0 for US915, etc.
#define BANDWIDTH_125_KHZ 125.0     // LoRa bandwidth in kHz.
#define A3MESH_PREAMBLE_LENGTH 8    // Length of LoRa transmission preamble in symbols.
#define A3MESH_SF 10                // LoRa spreading factor. Allowed values range from 6 to 12.
#define A3MESH_CR 5                 // 5: 4/5, 6: 4/6, 7: 4/7, 8: 4/8
#define A3MESH_SYNC_WORD 0x12       // LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
#define A3MESH_POWER 14             // Transmission output power in dBm. Allowed values range from 2 to 17 dBm.

#define A3MESH_MAX_PACKET_SIZE 100  // Maximum size of an A3MESH packet in bytes.

#define CS_PIN    8                 // Chip Select pin
#define IRQ_PIN   14                // Interrupt Request pin
#define RST_PIN   12                // Reset pin
#define DIO1_PIN  13                // DIO1 pin

#endif // CONFIG_H