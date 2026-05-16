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

            if (rxBLEQueue != nullptr)
            {
                Serial.printf("[BLE] RX: %s\n", buffer);
                xQueueSend(rxBLEQueue, buffer, 0);
            }
        }
    }

    BLEDriver::BLEDriver()
    {
    }

    bool BLEDriver::setup()
    {
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



}