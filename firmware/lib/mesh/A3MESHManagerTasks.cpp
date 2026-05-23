#include "A3MESHManagerTasks.h"

extern lib::mesh::A3MESHManager a3mesh;

void vNeighborCheckTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5000);

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        a3mesh.rm_tout_neigbhors();
    }
}