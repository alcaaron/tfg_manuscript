#pragma once
#include "A3MESHManager.hh"

/*!
 * \brief FreeRTOS task that periodically evicts timed-out entries from the
 *        neighbor table by calling A3MESHManager::rm_tout_neigbhors().
 *
 * Runs at a fixed period of NODE_TIMEOUT / 10 ms using vTaskDelayUntil,
 * which guarantees drift-free execution regardless of processing time.
 * Runs forever; does not return.
 *
 * \param [IN] pvParameters  Unused (pass NULL).
 */
void vNeighborCheckTask(void *pvParameters);
