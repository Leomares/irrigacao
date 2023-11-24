#include <Arduino.h>
#include "Memory.h"
#include "Controller.h"
#include "BLEHandler.h"

const char *S_WIFI_UUID = "2a38798e-8a3e-11ee-b9d1-0242ac120002";
const char *C_WIFI_W_UUID = "2a387bdc-8a3e-11ee-b9d1-0242ac120002";
const char *C_WIFI_R_UUID = "2a387d08-8a3e-11ee-b9d1-0242ac120002";

const char *S_PROFILE_UUID = "4e0b4756-8a3e-11ee-b9d1-0242ac120002";
const char *C_PROFILE_W_UUID = "4e0b4a62-8a3e-11ee-b9d1-0242ac120002";
const char *C_PROFILE_R_UUID = "4e0b4bde-8a3e-11ee-b9d1-0242ac120002";

const char *S_CONTROL_UUID[4] = {
    "8f904730-8a3e-11ee-b9d1-0242ac120002",
    "d6b23984-8a3e-11ee-b9d1-0242ac120002",
    "d6b23ee8-8a3e-11ee-b9d1-0242ac120002",
    "d6b24050-8a3e-11ee-b9d1-0242ac120002"};

const char *C_CONTROL_W_UUID[4] = {
    "8f904a32-8a3e-11ee-b9d1-0242ac120002",
    "f91de95a-8a3e-11ee-b9d1-0242ac120002",
    "f91decde-8a3e-11ee-b9d1-0242ac120002",
    "f91deed2-8a3e-11ee-b9d1-0242ac120002"};

const char *C_CONTROL_R_UUID[4] = {
    "8f904b86-8a3e-11ee-b9d1-0242ac120002",
    "c6a16c68-8a3e-11ee-b9d1-0242ac120002",
    "c6a1700a-8a3e-11ee-b9d1-0242ac120002",
    "c6a1719a-8a3e-11ee-b9d1-0242ac120002"};

extern Controller controllers[4];

// https://registry.platformio.org/libraries/h2zero/NimBLE-Arduino/examples/NimBLE_Server/NimBLE_Server.ino

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */

/** Handler class for characteristic actions */
// test

void setProfileInfo(String profileString)
{
    Profile *currentProfile = new Profile;
    int index;
    // deserialize
    Memory::setProfile(index, *currentProfile);
    delete currentProfile;
    return;
}

void setWiFiInfo(String wifiString)
{
    String ssid, password;
    // deserialize
    Memory::setWiFiConfig(ssid, password);
    return;
}

void setControllerInfo(int controlIndex, String controllerString)
{
    bool inUseControl;
    int profileIndexControl;
    // deserialize
    controllers[controlIndex].setProfile(profileIndexControl);
    if (!controllers[controlIndex].getInUse() && inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() + 1);
        controllers[controlIndex].setInUse();
    }
    else if (!controllers[controlIndex].getInUse() && !inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() - 1);
    }
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
        if (pCharacteristic->getUUID().toString() == C_PROFILE_W_UUID)
        {
            setProfileInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_W_UUID)
        {
            setWiFiInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_W_UUID[0])
        {
            setControllerInfo(0, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_W_UUID[1])
        {
            setControllerInfo(1, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_W_UUID[2])
        {
            setControllerInfo(2, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_W_UUID[3])
        {
            setControllerInfo(3, pCharacteristic->getValue());
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

NimBLEServer *BLEHandler::pServer = NimBLEDevice::createServer();

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

    pWifiService = BLEHandler::pServer->createService(S_WIFI_UUID);
    pWifiWriteCharacteristic = pWifiService->createCharacteristic(C_WIFI_W_UUID, NIMBLE_PROPERTY::WRITE);

    pWifiReadCharacteristic = pWifiService->createCharacteristic(C_WIFI_R_UUID, NIMBLE_PROPERTY::READ);

    pWifiWriteCharacteristic->setCallbacks(&chrCallbacks);
    pWifiReadCharacteristic->setCallbacks(&chrCallbacks);

    pProfileService = BLEHandler::pServer->createService(S_PROFILE_UUID);
    pProfileWriteCharacteristic = pProfileService->createCharacteristic(
        C_PROFILE_W_UUID,
        NIMBLE_PROPERTY::WRITE);

    pProfileReadCharacteristic = pProfileService->createCharacteristic(
        C_PROFILE_R_UUID,
        NIMBLE_PROPERTY::READ);

    pProfileWriteCharacteristic->setCallbacks(&chrCallbacks);
    pProfileReadCharacteristic->setCallbacks(&chrCallbacks);

    for (int i = 0; i < 4; i++)
    {
        pControllerService[i] = BLEHandler::pServer->createService(S_CONTROL_UUID[i]);
        pControllerWriteCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_W_UUID[i], NIMBLE_PROPERTY::WRITE);
        pControllerReadCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_R_UUID[i], NIMBLE_PROPERTY::READ);
        pControllerWriteCharacteristic[i]->setCallbacks(&chrCallbacks);
        pControllerReadCharacteristic[i]->setCallbacks(&chrCallbacks);
    }

    /** Start the services when finished creating all Characteristics and Descriptors */
    pWifiService->start();
    pProfileService->start();
    for (int i = 0; i < 4; i++)
    {
        pControllerService[i]->start();
    }

    pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pWifiService->getUUID());
    pAdvertising->addServiceUUID(pProfileService->getUUID());
    for (int i = 0; i < 4; i++)
    {
        pAdvertising->addServiceUUID(pControllerService[i]->getUUID());
    }
    /** If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("Advertising Started");
}