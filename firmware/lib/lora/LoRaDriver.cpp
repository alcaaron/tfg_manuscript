#include "LoRaDriver.h"
#include <config.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace lib::lora
{
    LoRaDriver::LoRaDriver()
    {
        this->radio = nullptr;
        receivedFlag = false;
    }

    bool LoRaDriver::begin(const lora::A3MESHConfig &config)
    {
        ESP_LOGV(A3M_TAG, "Initializing A3MESH v%s", A3M_VERSION);

        this->config = config;

        // Set module configuration
        this->radio = new SX1262(new Module(
            config.cs_pin,
            config.irq_pin,
            config.rst_pin,
            config.dio1_pin));

        // Initialize the radio with the specified configuration
        int16_t state = this->radio->begin(
            config.frequency,
            config.bandwidth,
            config.sf,
            config.cr,
            config.sync_word,
            config.power,
            config.preamble_len);

        if (state != RADIOLIB_ERR_NONE)
        {
            ESP_LOGE(A3M_TAG, "Failed to initialize LoRa module: %d", state);
            return false;
        }

        this->rxQueue = xQueueCreate(RX_LORA_QUEUE_SIZE, sizeof(RxMessage_t));
        this->txQueue = xQueueCreate(TX_LORA_QUEUE_SIZE, sizeof(lib::packet::A3MESHPacket_t));

        if (!this->rxQueue || !this->txQueue)
        {
            ESP_LOGE(A3M_TAG, "Failed to create queues");
            return false;
        }

        return true;
    }

    bool LoRaDriver::sendRawPacket(uint8_t *data, size_t len)
    {
        if (len > A3MESH_MAX_PACKET_SIZE)
            return false;

        int16_t state = this->radio->transmit(data, len);
        Serial.println("Raw packet sent!");
        return state == RADIOLIB_ERR_NONE;
    }

    bool LoRaDriver::sendPacket(const lib::packet::A3MESHPacket_t &packet)
    {
        uint8_t buffer[A3MESH_MAX_PACKET_SIZE];
        size_t total_len = 0;

        // serialize packet
        buffer[total_len++] = packet.pkt_id;
        buffer[total_len++] = packet.src;
        buffer[total_len++] = packet.node_id;
        buffer[total_len++] = packet.dst;
        buffer[total_len++] = packet.ttl;
        buffer[total_len++] = packet.type;
        buffer[total_len++] = packet.flags;
        buffer[total_len++] = packet.payload_len;
        memcpy(&buffer[total_len], packet.payload, packet.payload_len);

        total_len += packet.payload_len;

        if (packet.payload_len > A3MESH_MAX_PAYLOAD)
            return false;

        int16_t state = this->radio->transmit(buffer, total_len);

        if (state != RADIOLIB_ERR_NONE)
            return false;

        Serial.println("Packet sent!");
        Serial.print("Payload: ");
        for (size_t i = 0; i < packet.payload_len; i++)
        {
            Serial.print((char)packet.payload[i]);
        }
        Serial.println();

        return true;
    }

    volatile bool LoRaDriver::receivedFlag = false;

    bool LoRaDriver::startReceive()
    {
        int16_t state = this->radio->startReceive();

        return state == RADIOLIB_ERR_NONE;
    }

    void LoRaDriver::clearDioActions()
    {
        this->radio->clearDio1Action();
    }

    void LoRaDriver::setPacketReceivedAction()
    {
        this->radio->setPacketReceivedAction([]()
                                             { receivedFlag = true; });
    }

    bool LoRaDriver::checkPacketReceived()
    {
        return this->receivedFlag;
    }

    int LoRaDriver::readData(uint8_t *buffer, size_t len)
    {
        if (!this->receivedFlag)
            return 0;

        this->receivedFlag = false;

        int packetSize = this->radio->getPacketLength();

        if (packetSize == 0)
            return 0;

        if (packetSize > len)
            packetSize = len;

        int state = this->radio->readData(buffer, packetSize);

        this->radio->startReceive();

        if (state != RADIOLIB_ERR_NONE)
            return -1;

        return packetSize;
    }

    void LoRaDriver::rxLoRaPacketTask(void *pvParameters)
    {
        LoRaDriver *driver = static_cast<LoRaDriver *>(pvParameters);

        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xFrequency = POLLING_INTERVAL_MS / portTICK_PERIOD_MS;

        for (;;)
        {
            vTaskDelayUntil(&xLastWakeTime, xFrequency);

            if (driver->checkPacketReceived())
            {
                uint8_t buffer[RX_BUFFER_SIZE];
                int len = driver->readData(buffer, sizeof(buffer));

                if (len > sizeof(lib::packet::A3MESHHeader))
                {
                    RxMessage_t msg;
                    memcpy(msg.data, buffer, len);
                    msg.len = len;

                    xQueueSend(driver->rxQueue, &msg, portMAX_DELAY);
                }
            }
        }
    }

    void LoRaDriver::txLoRaPacketTask(void *pvParameters)
    {
        LoRaDriver *driver = static_cast<LoRaDriver *>(pvParameters);
        lib::packet::A3MESHPacket_t packet;

        for (;;)
        {
            if (xQueueReceive(driver->txQueue, &packet, portMAX_DELAY) == pdTRUE)
            {
                driver->sendPacket(packet);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    void LoRaDriver::processLoRaPacketTask(void *pvParameters)
    {
        LoRaDriver *driver = static_cast<LoRaDriver *>(pvParameters);

        RxMessage_t msg;

        for (;;)
        {
            if (xQueueReceive(driver->rxQueue, &msg, portMAX_DELAY) == pdTRUE)
            {
                Serial.println("Processing received packet...");

                for (size_t i = 0; i < msg.len; i++)
                {
                    Serial.print(msg.data[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();

                lib::packet::A3MESHHeader pktHeader;
                pktHeader.type = msg.data[0];
                pktHeader.version = msg.data[1];
                pktHeader.length = (msg.data[3] << 8) | msg.data[2];

                if (pktHeader.length == 0)
                {
                    Serial.println("Invalid packet");
                    continue;
                }

                uint8_t payload[pktHeader.length];
                memcpy(payload, &msg.data[4], pktHeader.length);

                lib::packet::A3MESHPacket_t pkt = {
                    .pkt_id = ((uint16_t)payload[1] << 8) | payload[0],
                    .node_id = ((uint16_t)payload[3] << 8) | payload[2],
                    .src = ((uint16_t)payload[5] << 8) | payload[4],
                    .dst = ((uint16_t)payload[7] << 8) | payload[6],
                    .ttl = payload[8],
                    .type = payload[9],
                    .flags = payload[10],
                    .payload_len = payload[11]};

                memcpy(pkt.payload, &payload[12], pkt.payload_len);

                Serial.println("Packet parsed!");
                Serial.print("pkt_id: ");
                Serial.print(pkt.pkt_id);
                Serial.print(", node_id: ");
                Serial.print(pkt.node_id);
                Serial.print(", src: ");
                Serial.print(pkt.src);
                Serial.print(", dst: ");
                Serial.print(pkt.dst);
                Serial.print(", ttl: ");
                Serial.print(pkt.ttl);
                Serial.print(", type: ");
                Serial.print(pkt.type);
                Serial.print(", flags: ");
                Serial.print(pkt.flags);
                Serial.print(", payload_len: ");
                Serial.println(pkt.payload_len);
                Serial.print("Payload content: ");
                for (size_t i = 0; i < pkt.payload_len; i++)                {
                    Serial.print((char)pkt.payload[i]);
                    Serial.print(" ");
                }
                Serial.println();
            }
        }
    }

    bool LoRaDriver::startTasks()
    {
        int result = xTaskCreate(
            rxLoRaPacketTask,
            "LoRa RX",
            4096,
            this,
            1,
            NULL);

        if (result != pdPASS)
        {
            ESP_LOGE(A3M_TAG, "Failed to create LoRa RX Task");
            return false;
        }

        result = xTaskCreate(
            txLoRaPacketTask,
            "LoRa TX",
            4096,
            this,
            1,
            NULL);

        if (result != pdPASS)
        {
            ESP_LOGE(A3M_TAG, "Failed to create LoRa TX Task");
            return false;
        }

        result = xTaskCreate(
            processLoRaPacketTask,
            "LoRa PROCESS",
            4096,
            this,
            1,
            NULL);
        
        if (result != pdPASS)
        {
            ESP_LOGE(A3M_TAG, "Failed to create LoRa Process Packet Task");
            return false;
        }

        return true;
    }

} // namespace lib::lora