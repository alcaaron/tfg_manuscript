#include "BLEDriver.hh"
#include <Arduino.h>

namespace lib::ble
{
    void ServerCallbacks::onConnect(BLEServer *pServer)
    {
        Serial.println("[BLE] Device Connected!");
        driver->_deviceConnected = true;
    }

    void ServerCallbacks::onDisconnect(BLEServer *pServer)
    {
        driver->_deviceConnected = false;
        pServer->startAdvertising();
    }

    void RXCallbacks::onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();

        if (!rxValue.empty())
        {
            char buffer[BLE_BUFFER_SIZE] = {0};
            strncpy(buffer, rxValue.c_str(), sizeof(buffer) - 1);

            if (xBLERxQueue != nullptr)
            {
                Serial.printf("[BLE] RX: %s", buffer);
                xQueueSend(xBLERxQueue, buffer, 0);
            }
        }
    }

    BLEDriver::BLEDriver()
    {
        _deviceConnected = false;
    }

    bool BLEDriver::setup()
    {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BT);

        char suffix[16];
        snprintf(
            suffix,
            sizeof(suffix),
            "_%02X:%02X:%02X",
            mac[3],
            mac[4],
            mac[5]
        );

        String deviceName = String(BLE_DEVICE_NAME) + suffix;

        BLEDevice::init(deviceName.c_str());

        // Crear servidor
        BLEServer *pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        // Crear servicio
        BLEService *pService = pServer->createService(SERVICE_UUID);

        // TX (notify > móvil)
        _txCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_TX,
            BLECharacteristic::PROPERTY_NOTIFY);
        _txCharacteristic->addDescriptor(new BLE2902());

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



    bool BLEDriver::is_connected()
    {
        return _deviceConnected;
    }

    void BLEDriver::shutdown()
    {
        BLEDevice::deinit();
    }

    bool BLEDriver::send(const char *txBuffer)
    {
        if (!_deviceConnected || _txCharacteristic == nullptr)
            return false;

        _txCharacteristic->setValue((uint8_t *)txBuffer, strlen(txBuffer));
        _txCharacteristic->notify();

        return true;
    }



}