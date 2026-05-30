#pragma once
#include "LoRaDriver.hh"
#include "LoRaQueues.h"

void vLoRaRxTask(void *pvParameters);
void vLoRaTxTask(void *pvParameters);