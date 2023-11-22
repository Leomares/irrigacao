#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include <NimBLEDevice.h>

class BLEHandler
{
public:
    BLEHandler();

    static void setProfileInfo(String profileString);

    static void setWiFiInfo(String wifiString);

    static void setControllerInfo(int controlIndex, String controllerString);

private:
    int connected;

    static NimBLEServer *pServer;

    NimBLEService *pWifiService;
    NimBLEService *pProfileService;
    NimBLEService *pControllerService[4];

    NimBLECharacteristic *pWifiWriteCharacteristic;
    NimBLECharacteristic *pWifiReadCharacteristic;

    NimBLECharacteristic *pProfileWriteCharacteristic;
    NimBLECharacteristic *pProfileReadCharacteristic;

    NimBLECharacteristic *pControllerWriteCharacteristic[4];
    NimBLECharacteristic *pControllerReadCharacteristic[4];

    NimBLEAdvertising *pAdvertising;
};

#endif