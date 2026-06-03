#pragma once
#include <Arduino.h>

/*!
 * \brief FreeRTOS queue for incoming BLE messages (mobile → firmware).
 *
 * Items are char[BLE_BUFFER_SIZE] strings in AT-command format.
 * Written by RXCallbacks::onWrite(); consumed by BLE2LoRaTask.
 */
extern QueueHandle_t xBLERxQueue;

/*!
 * \brief FreeRTOS queue for outgoing BLE messages (firmware → mobile).
 *
 * Items are char[BLE_BUFFER_SIZE] strings in AT-command response format.
 * Written by the mesh pipeline; drained by vBLETxTask.
 */
extern QueueHandle_t xBLETxQueue;
