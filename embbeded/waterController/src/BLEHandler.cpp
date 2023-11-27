#define CONTROLLER_USE 1
#define API_USE 1
#define BLE_USE 1
#define STRING_UUID 1

#if BLE_USE

#include <Arduino.h>
#include <sstream>
#include "Memory.h"
#include "WiFiHandler.h"
#include "Controller.h"
#include "BLEHandler.h"

static NimBLEServer *pServer;
static NimBLEService *pWifiService;
static NimBLEService *pProfileService;
static NimBLEService *pControllerService[4];
static NimBLECharacteristic *pWiFiStatusCharacteristic;
static NimBLECharacteristic *pWiFiSSIDCharacteristic;
static NimBLECharacteristic *pWiFiPWORDCharacteristic;
static NimBLECharacteristic *pProfileWriteCharacteristic;
// NimBLECharacteristic *pProfileReadCharacteristic;
static NimBLECharacteristic *pControllerWriteCharacteristic[4];
static NimBLECharacteristic *pControllerReadCharacteristic[4];
static NimBLEAdvertising *pAdvertising;

#if STRING_UUID
const char *S_WIFI_UUID = "2a38798e-8a3e-11ee-b9d1-0242ac120002";
const char *C_WIFI_R_STATUS_UUID = "2a387d08-8a3e-11ee-b9d1-0242ac120002";
const char *C_WIFI_W_SSID_UUID = "2a387bdc-8a3e-11ee-b9d1-0242ac120002";
const char *C_WIFI_W_PWORD_UUID = "4e0b4bde-8a3e-11ee-b9d1-0242ac120002";

/*
 set a profile in a certain index. Serialization used:
    "int index,bool isOutside,int volume,int regularPeriod,int cooldownPeriod"
*/
const char *S_PROFILE_UUID = "4e0b4756-8a3e-11ee-b9d1-0242ac120002";
const char *C_PROFILE_W_UUID = "4e0b4a62-8a3e-11ee-b9d1-0242ac120002";

const char *S_CONTROL_UUID[4] = {
    "8f904730-8a3e-11ee-b9d1-0242ac120002",
    "d6b23984-8a3e-11ee-b9d1-0242ac120002",
    "d6b23ee8-8a3e-11ee-b9d1-0242ac120002",
    "d6b24050-8a3e-11ee-b9d1-0242ac120002"};

/*
characteristics to set a profile and enable controller. Serialization used:
    "bool setInUse,int controllerProfileIndex"
*/
const char *C_CONTROL_W_UUID[4] = {
    "8f904a32-8a3e-11ee-b9d1-0242ac120002",
    "f91de95a-8a3e-11ee-b9d1-0242ac120002",
    "f91decde-8a3e-11ee-b9d1-0242ac120002",
    "f91deed2-8a3e-11ee-b9d1-0242ac120002"};

// characteristics to read whether a controller is enable or not.
const char *C_CONTROL_R_UUID[4] = {
    "8f904b86-8a3e-11ee-b9d1-0242ac120002",
    "c6a16c68-8a3e-11ee-b9d1-0242ac120002",
    "c6a1700a-8a3e-11ee-b9d1-0242ac120002",
    "c6a1719a-8a3e-11ee-b9d1-0242ac120002"};
#else

const char *S_WIFI_UUID = "SERVICE_WIFI";
const char *C_WIFI_R_STATUS_UUID = "STATUS";
const char *C_WIFI_W_SSID_UUID = "SSID";
const char *C_WIFI_W_PWORD_UUID = "PWORD";

const char *S_PROFILE_UUID = "SERVICE_PROFILE";
const char *C_PROFILE_W_UUID = "LAST_PROFILE";

const char *S_CONTROL_UUID[4] = {
    "SERVICE_CONTROL_1",
    "SERVICE_CONTROL_2",
    "SERVICE_CONTROL_3",
    "SERVICE_CONTROL_4",
};

const char *C_CONTROL_W_UUID[4] = {
    "WRITE_CONTROL_1",
    "WRITE_CONTROL_2",
    "WRITE_CONTROL_3",
    "WRITE_CONTROL_4",
};

const char *C_CONTROL_R_UUID[4] = {
    "STATUS_CONTROL_1",
    "STATUS_CONTROL_2",
    "STATUS_CONTROL_3",
    "STATUS_CONTROL_4",
};
#endif
#if CONTROLLER_USE
extern Controller controllers[4];
#else
const int sensorPins[4] = {34, 35, 36, 39}; // input only pins
const int pumpPins[4] = {32, 33, 25, 26};
Controller controllers[4] = {
    Controller(sensorPins[0], pumpPins[0]),
    Controller(sensorPins[1], pumpPins[1]),
    Controller(sensorPins[2], pumpPins[2]),
    Controller(sensorPins[3], pumpPins[3])};
#endif

// https://registry.platformio.org/libraries/h2zero/NimBLE-Arduino/examples/NimBLE_Server/NimBLE_Server.ino

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */

/** Handler class for characteristic actions */
// test

void setProfileInfo(String profileString)
{
    Profile *currentProfile = new Profile;
    int index;

    float numbers[5];
    std::istringstream iss(profileString.c_str());

    char delimiter = ',';
    for (int i = 0; i < 4; ++i)
    {
        std::string numberStr;
        if (std::getline(iss, numberStr, delimiter))
        {
            numbers[i] = atof(numberStr.c_str());
        }
    }
    //"int index,bool isOutside,int volume,int regularPeriod,int cooldownPeriod"
    index = numbers[0];
    currentProfile->isOutside = numbers[1] > 0;
    currentProfile->volume = numbers[2];
    currentProfile->regularPeriod = numbers[3];
    currentProfile->cooldownPeriod = numbers[4];
    Memory::setProfile(index, *currentProfile);

    delete currentProfile;
    return;
}

void setSSIDInfo(String ssidString)
{
    String ssid, password;
    Memory::getWiFiConfig(&ssid, &password);
    ssid = ssidString;
    Memory::setWiFiConfig(ssid, password);
    return;
}
void setPasswordInfo(String passString)
{
    String ssid, password;
    Memory::getWiFiConfig(&ssid, &password);
    password = passString;
    Memory::setWiFiConfig(ssid, password);
    return;
}

void setControllerInfo(int controlIndex, String controllerString)
{
    bool inUseControl;
    int profileIndexControl;

    std::istringstream iss(controllerString.c_str());
    int numbers[2];

    char delimiter = ',';
    for (int i = 0; i < 2; ++i)
    {
        std::string numberStr;
        if (std::getline(iss, numberStr, delimiter))
        {
            numbers[i] = atof(numberStr.c_str());
        }
    }
    inUseControl = numbers[0] > 0;
    profileIndexControl = numbers[1];
    controllers[controlIndex].setProfile(profileIndexControl);
    if (!controllers[controlIndex].getInUse() && inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() + 1);
        controllers[controlIndex].setInUse();
    }
    else if (controllers[controlIndex].getInUse() && !inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() - 1);
    }
    return;
}

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic)
    {

        if (pCharacteristic->getUUID().toString() == C_WIFI_R_STATUS_UUID)
        {
            pCharacteristic->setValue(WiFiHandler::isConnected());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_UUID[0])
        {
            pCharacteristic->setValue(controllers[0].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_UUID[1])
        {
            pCharacteristic->setValue(controllers[1].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_UUID[2])
        {
            pCharacteristic->setValue(controllers[2].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_UUID[3])
        {
            pCharacteristic->setValue(controllers[3].getInUse());
        }
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead(), value: ");
        Serial.println((String)pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        if (pCharacteristic->getUUID().toString() == C_PROFILE_W_UUID)
        {
            setProfileInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_W_SSID_UUID)

        {
            setSSIDInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_W_PWORD_UUID)
        {
            setPasswordInfo(pCharacteristic->getValue());
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

BLEHandler::BLEHandler() {}

void BLEHandler::setup()
{
    Serial.println("Starting NimBLE Server");
    /** sets device name */
    NimBLEDevice::init("NimBLE-irrigacao");
    pServer = NimBLEDevice::createServer();
    /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
    NimBLEDevice::setPower(9); /** +9db */
#endif

    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    pWifiService = pServer->createService(S_WIFI_UUID);
    pWiFiStatusCharacteristic = pWifiService->createCharacteristic(
        C_WIFI_R_STATUS_UUID,
        NIMBLE_PROPERTY::READ);
    pWiFiSSIDCharacteristic = pWifiService->createCharacteristic(
        C_WIFI_W_SSID_UUID,
        // NIMBLE_PROPERTY::READ || // remove later
        NIMBLE_PROPERTY::WRITE);

    pWiFiPWORDCharacteristic = pWifiService->createCharacteristic(
        C_WIFI_W_PWORD_UUID,
        // NIMBLE_PROPERTY::READ || // remove later
        NIMBLE_PROPERTY::WRITE);

    pWiFiStatusCharacteristic->setCallbacks(&chrCallbacks);
    pWiFiSSIDCharacteristic->setCallbacks(&chrCallbacks);
    pWiFiPWORDCharacteristic->setCallbacks(&chrCallbacks);

    pProfileService = pServer->createService(S_PROFILE_UUID);
    pProfileWriteCharacteristic = pProfileService->createCharacteristic(
        C_PROFILE_W_UUID,
        NIMBLE_PROPERTY::READ ||
            NIMBLE_PROPERTY::WRITE);

    pProfileWriteCharacteristic->setCallbacks(&chrCallbacks);
    // pProfileReadCharacteristic->setCallbacks(&chrCallbacks);

    for (int i = 0; i < 4; i++)
    {
        pControllerService[i] = pServer->createService(S_CONTROL_UUID[i]);
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

#endif