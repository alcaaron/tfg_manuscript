#include "BLEDriver.h"
#include <Arduino.h>

namespace lib::ble
{
    void ServerCallbacks::onConnect(BLEServer *pServer)
    {
        driver->deviceConnected = true;
    }

    void ServerCallbacks::onDisconnect(BLEServer *pServer)
    {
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

            if (driver->bleRxQueue != nullptr)
            {
                Serial.printf("[BLE] RX: %s\n", buffer);
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
        this->bleTxQueue = xQueueCreate(10, sizeof(char[BLE_BUFFER_SIZE]));

        if (!this->bleRxQueue || !this->bleTxQueue)
        {
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
        // if (xTaskCreate(BLErxTask, "BLE RX Task", 4096, this, 1, NULL) != pdPASS)
        // {
        //     return false;
        // }

        if (xTaskCreate(BLEtxTask, "BLE TX Task", 4096, this, 1, NULL) != pdPASS)
        {
            return false;
        }

        if (xTaskCreate(lifeBitTask, "BLE Life Bit Task", 4096, this, 1, NULL) != pdPASS)
        {
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

    bool BLEDriver::send(const char *txBuffer)
    {
        if (!deviceConnected || txCharacteristic == nullptr)
            return false;

        txCharacteristic->setValue((uint8_t *)txBuffer, strlen(txBuffer));
        txCharacteristic->notify();

        return true;
    }

    void BLEDriver::BLEtxTask(void *pvParameters)
    {
        BLEDriver *driver = static_cast<BLEDriver *>(pvParameters);
        char txBuffer[BLE_BUFFER_SIZE] = {0};

        if (driver == nullptr)
        {
            vTaskDelete(NULL);
            return;
        }

        for (;;)
        {
            if (xQueueReceive(driver->bleTxQueue, &txBuffer, portMAX_DELAY) == pdTRUE)
            {
                if (!driver->deviceConnected || driver->txCharacteristic == nullptr)
                    continue;
                
                driver->send(txBuffer);
                Serial.printf("[BLE] TX: %s\n", txBuffer);
            }
        }
    }

    void BLEDriver::lifeBitTask(void *pvParameters)
    {
        BLEDriver *driver = static_cast<BLEDriver *>(pvParameters);

        if (driver == nullptr)
        {
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