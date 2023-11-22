#include <Arduino.h>
#include "Memory.h"
// #include "APIWrapper.h" // not used
#include "Controller.h"
#include "BLEHandler.h"

// define UUIDS for each characteristics/services
#define S_WIFI "0"
#define S_PROF "0"
#define S_CONT "0"
#define C_PROFILE_WRITE_UUID "0"
#define C_WIFI_WRITE_UUID "0"

// https://registry.platformio.org/libraries/h2zero/NimBLE-Arduino/examples/NimBLE_Server/NimBLE_Server.ino

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */

/** Handler class for characteristic actions */
// test

void BLEHandler::setProfileInfo(String profileString)
{
    Profile *currentProfile = new Profile;
    int index;
    // desserialize
    Memory::setProfile(index, *currentProfile);
    delete currentProfile;
    return;
}

void BLEHandler::setWifiInfo(String wifiString)
{
    String ssid, password;
    // desserialize
    Memory::setWiFiConfig(ssid, password);
    return;
}

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic)
    {
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        if (pCharacteristic->getUUID().toString() == C_PROFILE_WRITE_UUID)
        {
            BLEHandler::setProfileInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_WRITE_UUID)
        {
            BLEHandler::setWifiInfo(pCharacteristic->getValue());
        }
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onWrite(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
    };
    /** Called before notification or indication is sent,
     *  the value can be changed here before sending if desired.
     */
    void onNotify(NimBLECharacteristic *pCharacteristic)
    {
        Serial.println("Sending notification to clients");
    };
};

static CharacteristicCallbacks chrCallbacks;

BLEHandler::BLEHandler()
{

    Serial.println("Starting NimBLE Server");

    /** sets device name */
    NimBLEDevice::init("NimBLE-irrigacao");

    /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
    NimBLEDevice::setPower(9); /** +9db */
#endif

    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    pServer = NimBLEDevice::createServer();

    pWifiService = pServer->createService(S_WIFI); // put standard UUID later
    pWifiWriteCharacteristic = pWifiService->createCharacteristic(
        "WRITE",
        NIMBLE_PROPERTY::WRITE);

    pWifiReadCharacteristic = pWifiService->createCharacteristic(
        "READ",
        NIMBLE_PROPERTY::READ);

    pWifiWriteCharacteristic->setCallbacks(&chrCallbacks);
    pWifiReadCharacteristic->setCallbacks(&chrCallbacks);

    pProfileService = pServer->createService(S_PROF); // put standard UUID later
    pProfileWriteCharacteristic = pProfileService->createCharacteristic(
        "WRITE",
        NIMBLE_PROPERTY::WRITE);

    pProfileReadCharacteristic = pProfileService->createCharacteristic(
        "READ",
        NIMBLE_PROPERTY::READ);

    pProfileWriteCharacteristic->setCallbacks(&chrCallbacks);
    pProfileReadCharacteristic->setCallbacks(&chrCallbacks);

    pControllerService = pServer->createService(S_CONT); // put standard UUID later
    pControllerWriteCharacteristic = pControllerService->createCharacteristic(
        "WRITE",
        NIMBLE_PROPERTY::WRITE);

    pControllerReadCharacteristic = pControllerService->createCharacteristic(
        "READ",
        NIMBLE_PROPERTY::READ);

    pControllerWriteCharacteristic->setCallbacks(&chrCallbacks);
    pControllerReadCharacteristic->setCallbacks(&chrCallbacks);

    /** Start the services when finished creating all Characteristics and Descriptors */
    pWifiService->start();
    pProfileService->start();
    pControllerService->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pWifiService->getUUID());
    pAdvertising->addServiceUUID(pProfileService->getUUID());
    pAdvertising->addServiceUUID(pControllerService->getUUID());
    /** If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("Advertising Started");
}