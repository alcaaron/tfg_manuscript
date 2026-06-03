#pragma once
#include <Arduino.h>

/*!
 * \brief FreeRTOS queue for received LoRa frames (radio → mesh pipeline).
 *
 * Items are rx_LoRa_msg_t* pointers into the driver's static pool.
 * Zero-copy: the pointer is enqueued by vLoRaRxTask and consumed by LoRa2BLETask.
 */
extern QueueHandle_t xLoRaRxQueue;

/*!
 * \brief FreeRTOS queue for outgoing LoRa frames (mesh pipeline → radio).
 *
 * Items are tx_LoRa_msg_t structs copied by value.
 * Written by BLE2LoRaTask / handleForward; consumed by vLoRaTxTask.
 */
extern QueueHandle_t xLoRaTxQueue;
