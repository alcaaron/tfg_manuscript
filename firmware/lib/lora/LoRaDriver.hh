#pragma once

#include <LoRa.h>
#include <SPI.h>
#include "LoRaConfig.h"
#include <config.h>
#include "A3MESHPacket.h"
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

namespace lib::lora
{
    /*!
     * \brief Low-level LoRa radio driver for the A3MESH stack.
     *
     * Wraps the SX127x LoRa library to provide interrupt-driven reception,
     * duty-cycle enforcement (EU868 1%), and a zero-copy frame pool for
     * the FreeRTOS RX task.  A single instance is shared across tasks;
     * all public methods that touch shared state are thread-safe.
     */
    class LoRaDriver
    {
    public:
        LoRaDriver();

        /*!
         * \brief Received LoRa frame produced by vLoRaRxTask.
         *
         * Allocated from a static ring-buffer pool (alloc_frame()) so that
         * the raw bytes are never copied between the ISR context and the
         * mesh pipeline.
         */
        typedef struct
        {
            uint8_t rx_data[LORA_MAX_PACKET_SIZE]; /*!< Raw bytes read from the SX127x FIFO. */
            size_t  len;                           /*!< Number of valid bytes in rx_data[]. */
            int16_t rssi;                          /*!< Packet RSSI in dBm as reported by the radio. */
        } rx_LoRa_msg_t;

        /*!
         * \brief Outgoing LoRa frame consumed by vLoRaTxTask.
         *
         * Copied by value into xLoRaTxQueue.  delay_ms allows the mesh layer
         * to impose a jitter window before the actual transmission.
         */
        typedef struct
        {
            uint8_t  tx_data[LORA_MAX_PACKET_SIZE]; /*!< Serialised frame bytes to transmit. */
            size_t   len;                           /*!< Number of valid bytes in tx_data[]. */
            uint16_t delay_ms;                      /*!< Milliseconds to wait before transmitting (jitter). */
            uint16_t pkt_id;                        /*!< Mesh packet ID, used for passive-ACK suppression. */
            uint32_t src;                           /*!< Mesh source node ID, used for passive-ACK suppression. */
            bool     is_forward;                    /*!< True when this frame is a forwarded copy subject to passive-ACK. */
        } tx_LoRa_msg_t;

        /*!
         * \brief Initialises the SX127x radio with the given configuration.
         *
         * Resets the chip, configures SPI pins, attaches the DIO0 interrupt,
         * creates the RX binary semaphore, and puts the radio into continuous
         * receive mode.
         *
         * \param [IN] config  Radio parameters (frequency, SF, BW, CR, etc.).
         *
         * \retval true   Radio initialised successfully.
         * \retval false  LoRa.begin() failed (hardware not detected).
         */
        bool begin(const lora::A3MESHConfig &config);

        /*!
         * \brief Transmits a raw byte buffer over LoRa after duty-cycle check.
         *
         * Checks the EU868 1% duty-cycle budget before transmission.  If the
         * budget is exhausted the packet is dropped silently.  The radio is
         * returned to continuous receive mode after each successful TX.
         *
         * \param [IN] data  Reference to the first byte of the buffer.
         * \param [IN] len   Number of bytes to transmit.
         *
         * \retval true   Packet transmitted (or at least written to the FIFO).
         * \retval false  Packet exceeded LORA_MAX_PACKET_SIZE or duty-cycle limit reached.
         */
        bool send_raw_pkt(const uint8_t &data, size_t len);

        /*!
         * \brief DIO0 rising-edge ISR -- signals the RX semaphore to wake vLoRaRxTask.
         *
         * Must be placed in IRAM (IRAM_ATTR) to be callable during flash cache
         * misses on the ESP32.  Yields to a higher-priority task if one was
         * unblocked by the semaphore give.
         */
        static void IRAM_ATTR onLoRaRxIRQ();

        /*!
         * \brief Blocks the calling task until an RX interrupt has been signalled.
         *
         * Wraps xSemaphoreTake on the internal binary semaphore with
         * portMAX_DELAY, so the task consumes no CPU while the radio is idle.
         */
        static void wait_for_rx();

        /*!
         * \brief Allocates the next frame slot from the static pool (round-robin).
         *
         * The pool holds 8 slots; the caller fills rx_data[] and len before
         * posting the pointer to xLoRaRxQueue.  Slots are reused cyclically --
         * the consumer (mesh pipeline) must process each frame before the pool
         * wraps around (8 unprocessed frames).
         *
         * \retval rx_LoRa_msg_t*  Pointer to the next available pool slot.
         */
        static rx_LoRa_msg_t* alloc_frame();

        /*!
         * \brief Estimates the time-on-air for a packet of the given length.
         *
         * Uses the Semtech SX127x time-on-air formula (AN1200.13 rev2 §4)
         * with the SF, BW, CR, and preamble constants from config.h.
         * Assumes CRC enabled, explicit header mode, and DE=0 (valid for SF ≤ 10
         * at 125 kHz).
         *
         * \param [IN] pkt_len  Number of bytes in the LoRa payload.
         *
         * \retval uint32_t  Estimated time-on-air in milliseconds (ceiling).
         */
        static uint32_t compute_airtime_ms(size_t pkt_len);

    private:
        /*!< Binary semaphore given by onLoRaRxIRQ() and taken by wait_for_rx(). */
        static SemaphoreHandle_t _rx_sem;

        /*!< Static pool of RX frame slots shared across calls to alloc_frame(). */
        static rx_LoRa_msg_t pool[8];

        /*!< Next pool slot to allocate; wraps modulo 8 using bitwise AND. */
        static volatile uint8_t pool_idx;

        /*!
         * \brief Ring-buffer entry for duty-cycle tracking.
         */
        struct AirtimeEntry {
            uint32_t ts_ms;  /*!< Timestamp (millis()) when this TX began. */
            uint32_t dur_ms; /*!< Time-on-air of this TX in milliseconds. */
        };

        /*!< Ring buffer of the most recent LORA_AIRTIME_RECORDS TX durations.
         *   Accessed only from vLoRaTxTask -- no mutex required. */
        AirtimeEntry _airtime_log[LORA_AIRTIME_RECORDS] = {};

        /*!< Write index into _airtime_log[]; advances modulo LORA_AIRTIME_RECORDS. */
        uint8_t _airtime_log_idx = 0;

        /*!
         * \brief Checks whether a pending TX fits within the EU868 1% duty-cycle
         *        budget and, if so, records it in the airtime log.
         *
         * Scans _airtime_log[] for entries whose timestamp falls within the
         * LORA_DUTY_CYCLE_WINDOW_MS sliding window and sums their durations.
         *
         * \param [IN] airtime_ms  Estimated duration of the prospective TX.
         *
         * \retval true   TX is within budget; the entry has been recorded.
         * \retval false  Budget exhausted; the TX must be dropped.
         */
        bool check_and_record_airtime(uint32_t airtime_ms);
    };
} // namespace lib::lora
