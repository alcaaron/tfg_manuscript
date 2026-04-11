#include "BLEDriver.h"

namespace lib::ble
{
    BLEDriver::BLEDriver()
    {
    }

    bool BLEDriver::setup()
    {
        BLEDevice::init("A3MESH_NODE");
        BLEServer *pServer = BLEDevice::createServer();
        return true;
    }

    bool BLEDriver::isConnected()
    {
        // Implement connection status check if needed
        return false;
    }

    void BLEDriver::shutdown()
    {
        BLEDevice::deinit();
    }
}