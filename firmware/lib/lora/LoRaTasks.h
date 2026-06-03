#pragma once
#include "LoRaDriver.hh"
#include "LoRaQueues.h"

/*!
 * \brief FreeRTOS task that waits for an RX semaphore signalled by the DIO0 ISR,
 *        reads the incoming frame from the SX127x and pushes a pointer to
 *        xLoRaRxQueue (zero-copy via the driver's frame pool).
 *
 * Blocks on LoRaDriver::wait_for_rx() when the radio is idle.
 * Runs forever; does not return.
 *
 * \param [IN] pvParameters  Unused (pass NULL).
 */
void vLoRaRxTask(void *pvParameters);

/*!
 * \brief FreeRTOS task that drains xLoRaTxQueue, applies the per-packet
 *        jitter delay, checks the passive-ACK suppression flag, and calls
 *        LoRaDriver::send_raw_pkt() to transmit over the air.
 *
 * Blocks indefinitely on the queue when no frames are pending.
 * Runs forever; does not return.
 *
 * \param [IN] pvParameters  Unused (pass NULL).
 */
void vLoRaTxTask(void *pvParameters);
