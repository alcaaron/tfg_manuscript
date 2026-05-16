#pragma once
#include "LoRaDriver.h"
#include "LoRaQueues.h"

void rxLoRaPacketTask(void *pvParameters);
void txLoRaPacketTask(void *pvParameters);