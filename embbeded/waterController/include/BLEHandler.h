#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include <NimBLEDevice.h>

class BLEHandler
{
public:
    BLEHandler();

    static void setProfileInfo(int n, Controller controllerN);

    static void setWifiInfo(String ssid);

    private:
    int connected;

    static NimBLEServer* pServer;

    NimBLEService* pWifiService;
    NimBLEService* pProfileService;

    NimBLECharacteristic* pWifiWriteCharacteristic;
    NimBLECharacteristic* pWifiReadCharacteristic;

    NimBLECharacteristic* pProfileWriteCharacteristic;
    NimBLECharacteristic* pProfileReadCharacteristic;

    NimBLEAdvertising* pAdvertising;
};

#endif