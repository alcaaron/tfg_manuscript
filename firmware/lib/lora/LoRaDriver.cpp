#include "LoRaDriver.h"

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
        this->txQueue = xQueueCreate(TX_LORA_QUEUE_SIZE, sizeof(TxMessage_t));

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

        clearDioActions();
        this->radio->standby();
        int16_t state = this->radio->transmit(data, len);
        this->radio->startReceive();
        setPacketReceivedAction();
        Serial.print("[LORA] TX: ");
        for (size_t i = 0; i < len; i++)
        {
            if (data[i] < 0x10)
                Serial.print("0");

            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        return state == RADIOLIB_ERR_NONE;
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
        if (!receivedFlag)
            return 0;

        receivedFlag = false;

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
                Serial.print("[LORA] RX: ");
                for (size_t i = 0; i < len; i++)
                {
                    if (buffer[i] < 0x10)
                        Serial.print("0");

                    Serial.print(buffer[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();

                if (len > sizeof(lib::packet::A3MESHHeader_t))
                {
                    RxMessage_t msg;
                    memcpy(msg.rx_data, buffer, len);
                    msg.len = len;

                    xQueueSend(driver->rxQueue, &msg, portMAX_DELAY);
                }
            }
        }
    }

    void LoRaDriver::txLoRaPacketTask(void *pvParameters)
    {
        LoRaDriver *driver = static_cast<LoRaDriver *>(pvParameters);
        TxMessage_t msg;

        for (;;)
        {
            if (xQueueReceive(driver->txQueue, &msg, portMAX_DELAY) == pdTRUE)
            {
                driver->sendRawPacket(msg.tx_data, msg.len);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
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

        return true;
    }

} // namespace lib::lora