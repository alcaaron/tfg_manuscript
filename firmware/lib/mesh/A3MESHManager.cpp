#include "A3MESHManager.h"
#include <freertos/queue.h>
#include <freertos/task.h>

namespace lib::mesh
{

    A3MESHManager::A3MESHManager()
    {
        _mutex = xSemaphoreCreateMutex();
        if (!_mutex)
        {
            Serial.println("Mutex creation failed!");
            while (1)
                ;
        }
        _node_id = generateNodeID();
        Serial.printf("Node ID: %u\n", _node_id);
    }

    uint16_t A3MESHManager::getNodeID()
    {
        return _node_id;
    }

    uint16_t A3MESHManager::getNextMsgID()
    {

        uint16_t id;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        id = _msg_counter++;
        xSemaphoreGive(_mutex);

        return id;
    }

    uint32_t A3MESHManager::generateNodeID()
    {

        uint64_t mac = ESP.getEfuseMac();
        return (uint32_t)((mac >> 16) ^ mac);
    }

    void A3MESHManager::sendDiscovery()
    {
        const char *msg = "DISC";

        lib::lora::TxMessage_t lora_msg;
        lib::packet::A3MESHPacket::createLoRaMessage(
            A3M_VERSION,
            lib::packet::MessageType::MSG_TYPE_DISCOVERY,
            getNextMsgID(),
            _node_id,
            _node_id,
            0xFFFF, // Broadcast
            1,      // TTL
            0,      // Flags
            (const uint8_t *)msg,
            lora_msg.tx_data,
            lora_msg.len);
    }

    void A3MESHManager::sendDiscoveryTask(void *pvParameters)
    {
        A3MESHManager *manager = static_cast<A3MESHManager *>(pvParameters);
        for (;;)
        {
            manager->sendDiscovery();
            vTaskDelay(pdMS_TO_TICKS(30000) + (pdMS_TO_TICKS(rand() % 30000)));
        }
        vTaskDelete(NULL);
    }




}