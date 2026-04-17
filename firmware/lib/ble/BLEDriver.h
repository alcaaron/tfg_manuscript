#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <config.h>

namespace lib::ble
{
    class BLEDriver;

    class ServerCallbacks : public BLEServerCallbacks
    {
        public:
            ServerCallbacks(BLEDriver *driver) : driver(driver) {}

            void onConnect(BLEServer *pServer) override;
            void onDisconnect(BLEServer *pServer) override;
        private:
            BLEDriver *driver;
    };

    class RXCallbacks : public BLECharacteristicCallbacks
    {
    public:
        RXCallbacks(BLEDriver *driver) : driver(driver) {}

        void onWrite(BLECharacteristic *pCharacteristic) override;

    private:
        BLEDriver *driver;
    };

    class BLEDriver
    {
    public:
        BLEDriver();

        bool setup();

        bool isConnected();

        bool startTasks();

        void shutdown();

    private:
        BLECharacteristic *txCharacteristic = nullptr;

        QueueHandle_t bleRxQueue = nullptr;
        // QueueHandle_t bleTxQueue;

        bool deviceConnected = false;

        static void BLEprocessorTask(void *pvParameters);
        static void lifeBitTask(void *pvParameters);

        friend class ServerCallbacks;
        friend class RXCallbacks;
    };
}

#endif // BLE_DRIVER_H