#include "BLEDriver.h"

namespace lib::ble
{
    void ServerCallbacks::onConnect(BLEServer *pServer)
    {
        // Serial.println("Device connected");
        driver->deviceConnected = true;
    }

    void ServerCallbacks::onDisconnect(BLEServer *pServer)
    {
        // Serial.println("Device disconnected");
        driver->deviceConnected = false;
        pServer->startAdvertising();
    }

    void RXCallbacks::onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();

        if (!rxValue.empty())
        {
            char buffer[BLE_BUFFER_SIZE] = {0};
            strncpy(buffer, rxValue.c_str(), sizeof(buffer) - 1);

            // Serial.print("Received RX: ");
            // Serial.println(buffer);

            if (driver->bleRxQueue != nullptr)
            {
                xQueueSend(driver->bleRxQueue, buffer, 0);
            }
        }
    }

    BLEDriver::BLEDriver()
    {
    }

    bool BLEDriver::setup()
    {
        this->bleRxQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));

        if (!this->bleRxQueue)
        {
            // Serial.println("Failed to create BLE RX queue");
            return false;
        }

        BLEDevice::init(BLE_DEVICE_NAME);

        // Crear servidor
        BLEServer *pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        // Crear servicio
        BLEService *pService = pServer->createService(SERVICE_UUID);

        // TX (notify > móvil)
        txCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_TX,
            BLECharacteristic::PROPERTY_NOTIFY);
        txCharacteristic->addDescriptor(new BLE2902());

        // RX (write > desde móvil)
        BLECharacteristic *rxCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_RX,
            BLECharacteristic::PROPERTY_WRITE);
        rxCharacteristic->setCallbacks(new RXCallbacks(this));

        // Iniciar servicio
        pService->start();

        // Advertising
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->start();

        return true;
    }

    bool BLEDriver::startTasks()
    {
        if (xTaskCreate(BLEprocessorTask, "BLE Processor", 4096, this, 1, NULL) != pdPASS)
        {
            // Serial.println("Failed to create BLE Processor task");
            return false;
        }

        if (xTaskCreate(lifeBitTask, "Life Bit Task", 4096, this, 1, NULL) != pdPASS)
        {
            // Serial.println("Failed to create Life Bit task");
            return false;
        }

        return true;
    }

    bool BLEDriver::isConnected()
    {
        return deviceConnected;
    }

    void BLEDriver::shutdown()
    {
        BLEDevice::deinit();
    }

    void BLEDriver::BLEprocessorTask(void *pvParameters)
    {
        BLEDriver *driver = static_cast<BLEDriver *>(pvParameters);

        char rxBuffer[BLE_BUFFER_SIZE];

        if (driver == nullptr)
        {
            // Serial.println("BLE RX queue not initialized");
            vTaskDelete(NULL);
            return;
        }

        for (;;)
        {
            if (xQueueReceive(driver->bleRxQueue, rxBuffer, portMAX_DELAY) == pdTRUE)
            {
                // Serial.print("Processing BLE RX: ");
                // Serial.println(rxBuffer);

                if (driver->deviceConnected && driver->txCharacteristic)
                {
                    char txBuffer[64];
                    snprintf(txBuffer, sizeof(txBuffer), "Received: %s", rxBuffer);
                    driver->txCharacteristic->setValue(txBuffer);
                    driver->txCharacteristic->notify();
                }
            }
        }
    }

    void BLEDriver::lifeBitTask(void *pvParameters)
    {
        BLEDriver *driver = static_cast<BLEDriver *>(pvParameters);

        if (driver == nullptr)
        {
            // Serial.println("BLE Driver not initialized");
            vTaskDelete(NULL);
            return;
        }

        for (;;)
        {
            if (driver->deviceConnected && driver->txCharacteristic)
            {
                const char *msg = "alive";
                driver->txCharacteristic->setValue((uint8_t *)msg, strlen(msg));
                driver->txCharacteristic->notify();
            }

            vTaskDelay(pdMS_TO_TICKS(BLE_LIFE_BIT_INTERVAL_MS));
        }
    }

}