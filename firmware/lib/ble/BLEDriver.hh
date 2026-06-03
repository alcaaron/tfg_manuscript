#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <config.h>
#include "A3MESHPacket.h"
#include "BLEQueues.h"


namespace lib::ble
{
    class BLEDriver;

    /*!
     * \brief BLE server connection callbacks.
     *
     * Notifies BLEDriver when a central device connects or disconnects,
     * and restarts advertising automatically after a disconnection.
     */
    class ServerCallbacks : public BLEServerCallbacks
    {
        public:
            /*!
             * \brief Constructs the callback object bound to a BLEDriver instance.
             *
             * \param [IN] driver  Pointer to the owning BLEDriver.
             */
            ServerCallbacks(BLEDriver *driver) : driver(driver) {}

            /*!
             * \brief Called by the BLE stack when a central device connects.
             *
             * Sets the driver's connected flag to true.
             *
             * \param [IN] pServer  Pointer to the BLE server instance.
             */
            void onConnect(BLEServer *pServer) override;

            /*!
             * \brief Called by the BLE stack when the central device disconnects.
             *
             * Clears the driver's connected flag and restarts advertising.
             *
             * \param [IN] pServer  Pointer to the BLE server instance.
             */
            void onDisconnect(BLEServer *pServer) override;

        private:
            BLEDriver *driver;
    };

    /*!
     * \brief BLE RX characteristic write callbacks.
     *
     * Invoked by the BLE stack when the mobile app writes to the RX
     * characteristic.  The received string is forwarded to xBLERxQueue
     * for processing by the BLE2LoRaTask.
     */
    class RXCallbacks : public BLECharacteristicCallbacks
    {
    public:
        /*!
         * \brief Constructs the callback object bound to a BLEDriver instance.
         *
         * \param [IN] driver  Pointer to the owning BLEDriver.
         */
        RXCallbacks(BLEDriver *driver) : driver(driver) {}

        /*!
         * \brief Called by the BLE stack when the central writes to the RX
         *        characteristic.
         *
         * Copies the received value into a local buffer and posts it to
         * xBLERxQueue (non-blocking; drops the message if the queue is full).
         *
         * \param [IN] pCharacteristic  Pointer to the written characteristic.
         */
        void onWrite(BLECharacteristic *pCharacteristic) override;

    private:
        BLEDriver *driver;
    };

    /*!
     * \brief High-level BLE peripheral driver for the A3MESH node.
     *
     * Exposes a single NUS-compatible GATT service with one TX (notify) and
     * one RX (write) characteristic.  Received data is forwarded to
     * xBLERxQueue; outgoing data is sent via send().
     */
    class BLEDriver
    {
    public:
        /*!
         * \brief Constructs a BLEDriver with the device initially disconnected.
         */
        BLEDriver();

        /*!
         * \brief Initialises the BLE stack, creates the GATT service and
         *        characteristics, and starts advertising.
         *
         * \retval true   Initialisation succeeded.
         * \retval false  Initialisation failed (BLE stack error).
         */
        bool setup();

        /*!
         * \brief Returns whether a BLE central is currently connected.
         *
         * \retval true   A central device is connected.
         * \retval false  No central device is connected.
         */
        bool is_connected();

        /*!
         * \brief De-initialises the BLE stack and releases all resources.
         */
        void shutdown();

        /*!
         * \brief Sends a null-terminated string to the connected central via
         *        a BLE notification on the TX characteristic.
         *
         * \param [IN] txBuffer  Null-terminated string to transmit.
         *
         * \retval true   Notification sent successfully.
         * \retval false  No device connected or TX characteristic not ready.
         */
        bool send(const char *txBuffer);

        /*!
         * \brief Returns a pointer to the TX BLE characteristic.
         *
         * \retval BLECharacteristic*  Pointer to the TX characteristic,
         *                             or nullptr if setup() has not been called.
         */
        BLECharacteristic* getTxCharacteristic() const { return _txCharacteristic; }

    private:
        BLECharacteristic *_txCharacteristic = nullptr;

        bool _deviceConnected = false;

        friend class ServerCallbacks;
        friend class RXCallbacks;
    };
}

#endif // BLE_DRIVER_H
