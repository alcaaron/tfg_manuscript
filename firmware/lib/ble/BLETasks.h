#pragma once
#include "BLEDriver.hh"
#include "BLEQueues.h"

/*!
 * \brief FreeRTOS task that drains xBLETxQueue and transmits each message
 *        to the connected BLE client via BLEDriver::send().
 *
 * Blocks indefinitely on the queue when no messages are pending.
 * Runs forever; does not return.
 *
 * \param [IN] pvParameters  Unused (pass NULL).
 */
void vBLETxTask(void *pvParameters);
