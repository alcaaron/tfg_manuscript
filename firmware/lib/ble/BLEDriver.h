#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

namespace lib::ble
{
    class BLEDriver
    {
    public:
        BLEDriver();

        bool setup();

        bool isConnected();

        void shutdown();


    private:
        
    };
}

#endif // BLE_DRIVER_H