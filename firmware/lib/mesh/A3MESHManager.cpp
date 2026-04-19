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

    bool A3MESHManager::begin()
    {

        if (xTaskCreate(sendDiscoveryTask, "DiscoveryTask", 4096, this, 1, NULL) != pdPASS)
            return false;

        if (xTaskCreate(BLE2LoRaTask, "BLE2LoRaTask", 4096, this, 1, NULL) != pdPASS)
            return false;

        if (xTaskCreate(LoRa2BLETask, "LoRa2BLETask", 4096, this, 1, NULL) != pdPASS)
            return false;

        return true;
    }

    void A3MESHManager::attachBLE(lib::ble::BLEDriver *b)
    {
        ble = b;
    }

    void A3MESHManager::attachLoRa(lib::lora::LoRaDriver *l)
    {
        lora = l;
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

        if (lora)
        {
            xQueueSend(lora->getTxQueue(), &lora_msg, 0);
        }
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

    void A3MESHManager::BLE2LoRaTask(void *pvParameters)
    {
        A3MESHManager *manager = static_cast<A3MESHManager *>(pvParameters);
        char rxBuffer[BLE_BUFFER_SIZE] = {0};

        if (manager == nullptr)
        {
            vTaskDelete(NULL);
            return;
        }

        for (;;)
        {
            if (xQueueReceive(manager->ble->getRxQueue(), &rxBuffer, portMAX_DELAY) == pdTRUE)
            {
                /*
                Expected send format: AT+SEND=<payload>[,<dst>]
                */
                if (strncmp(rxBuffer, "AT+SEND=", 8) == 0)
                {
                    char *payload_start = rxBuffer + 8;
                    char *dst_str = nullptr;

                    char *comma = strchr(payload_start, ',');
                    uint16_t dst = 0xFFFF; // default broadcast

                    if (comma != nullptr)
                    {
                        *comma = '\0';
                        dst_str = comma + 1;

                        dst = (uint16_t)strtol(dst_str, nullptr, 0);
                    }

                    const char *msg = payload_start;
                    size_t msg_len = strlen(msg);

                    if (msg_len == 0 || msg_len > A3MESH_MAX_PAYLOAD)
                    {
                        continue;
                    }

                    lib::lora::TxMessage_t lora_msg;

                    lib::packet::A3MESHPacket::createLoRaMessage(
                        A3M_VERSION,
                        lib::packet::MessageType::MSG_TYPE_DISCOVERY,
                        manager->getNextMsgID(),
                        manager->_node_id, // node_id
                        manager->_node_id, // src
                        dst,
                        MAX_TTL,
                        0,
                        (const uint8_t *)msg,
                        lora_msg.tx_data,
                        lora_msg.len);

                    if (manager->lora)
                    {
                        xQueueSend(manager->lora->getTxQueue(), &lora_msg, 0);
                    }
                }
            }
        }
    }

    void A3MESHManager::LoRa2BLETask(void *pvParameters)
    {
        A3MESHManager *manager = static_cast<A3MESHManager *>(pvParameters);
        uint8_t rxBuffer[A3MESH_MAX_PACKET_SIZE] = {0};

        if (manager == nullptr)
        {
            vTaskDelete(NULL);
            return;
        }

        for (;;)
        {
            if (xQueueReceive(manager->lora->getRxQueue(), rxBuffer, portMAX_DELAY) == pdTRUE)
            {
                if (!manager->ble)
                    continue;

                lib::packet::A3MESHHeader_t header;
                lib::packet::A3MESHPacket_t pkt;

                bool ok = lib::packet::A3MESHPacket::deserializePacket(
                    rxBuffer,
                    A3MESH_MAX_PACKET_SIZE,
                    header,
                    pkt);

                if (!ok)
                {
                    continue;
                }
                char bleBuffer[BLE_BUFFER_SIZE] = {0};
                if (pkt.payload_len >= (BLE_BUFFER_SIZE - 5))
                    continue;
                int len = snprintf(
                    bleBuffer,
                    sizeof(bleBuffer),
                    "+RCV=%.*s",
                    pkt.payload_len,
                    pkt.payload);

                if (len < 0 || len >= BLE_BUFFER_SIZE)
                    continue;

                xQueueSend(manager->ble->getTxQueue(), bleBuffer, 0);
            }
        }
    }
}