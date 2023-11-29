#define SS_USE 0

#include <Arduino.h>
#include <NimBLEDevice.h>
#if SS_USE
#include <sstream>
#endif
#include "Memory.h"
#include "WiFiHandler.h"
#include "Controller.h"
#include "BLEHandler.h"

static NimBLEServer *pServer;

static NimBLEService *pWiFiService;
const char *S_WIFI_UUID = "2a38798e-8a3e-11ee-b9d1-0242ac120002";

// Read whether the microcontroller is connected to a WiFi network.
static NimBLECharacteristic *pWiFiStatusCharacteristic;
const char *C_WIFI_R_STATUS_UUID = "2a387d08-8a3e-11ee-b9d1-0242ac120002";

// Set SSID and password to the microcontroller.
static NimBLECharacteristic *pWiFiConfigCharacteristic;
const char *C_WIFI_W_CONFIG_UUID = "2a387bdc-8a3e-11ee-b9d1-0242ac120002";

static NimBLEService *pProfileService;
const char *S_PROFILE_UUID = "4e0b4756-8a3e-11ee-b9d1-0242ac120002";

/*
 set a profile in a certain index. Serialization used:
    "int index,bool isOutside,int volume,int regularPeriod,int cooldownPeriod"
*/
static NimBLECharacteristic *pProfileWriteCharacteristic;
const char *C_PROFILE_W_UUID = "4e0b4a62-8a3e-11ee-b9d1-0242ac120002";

static NimBLEService *pControllerService[4];
const char *S_CONTROL_UUID[4] = {
    "8f904730-8a3e-11ee-b9d1-0242ac120002",
    "d6b23984-8a3e-11ee-b9d1-0242ac120002",
    "d6b23ee8-8a3e-11ee-b9d1-0242ac120002",
    "d6b24050-8a3e-11ee-b9d1-0242ac120002"};

/*
characteristics to set a profile and enable controller. Serialization used:
    "bool setInUse,int controllerProfileIndex"
*/
static NimBLECharacteristic *pControllerWriteCharacteristic[4];
const char *C_CONTROL_W_UUID[4] = {
    "8f904a32-8a3e-11ee-b9d1-0242ac120002",
    "f91de95a-8a3e-11ee-b9d1-0242ac120002",
    "f91decde-8a3e-11ee-b9d1-0242ac120002",
    "f91deed2-8a3e-11ee-b9d1-0242ac120002"};

// characteristic to read whether a controller is enable or not.
static NimBLECharacteristic *pControllerReadStatusCharacteristic[4];
const char *C_CONTROL_R_STATUS_UUID[4] = {
    "8f904b86-8a3e-11ee-b9d1-0242ac120002",
    "c6a16c68-8a3e-11ee-b9d1-0242ac120002",
    "c6a1700a-8a3e-11ee-b9d1-0242ac120002",
    "c6a1719a-8a3e-11ee-b9d1-0242ac120002"};

// characteristic to read the current profile of a controller.
static NimBLECharacteristic *pControllerReadProfileCharacteristic[4];
const char *C_CONTROL_R_PROFILE_UUID[4] = {
    "817bf150-8eec-11ee-b9d1-0242ac120002",
    "817bf39e-8eec-11ee-b9d1-0242ac120002",
    "817bf4b6-8eec-11ee-b9d1-0242ac120002",
    "817bf5ba-8eec-11ee-b9d1-0242ac120002"};

static NimBLEAdvertising *pAdvertising;

extern Controller controllers[4];

// https://registry.platformio.org/libraries/h2zero/NimBLE-Arduino/examples/NimBLE_Server/NimBLE_Server.ino

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */

/** Handler class for characteristic actions */

// Serialize profile concatenating values with commas.
String getProfileInfo(int controllerIndex)
{
    Serial.println("\nFetching profile...");
    int profileIndex = controllers[controllerIndex].getProfileIndex();
    Serial.println("Profile " + String(controllerIndex) + "from controller " + String(profileIndex));
    Profile currentProfile = Memory::getProfile(profileIndex);
    String serializedProfile =
        String(currentProfile.isOutside) + "," +
        String(currentProfile.volume) + "," +
        String(currentProfile.regularPeriod) + "," +
        String(currentProfile.cooldownPeriod);
    return serializedProfile;
}

/*
Deserialize string fetched from BLE characteristic for setting a profile. The pattern used is the following:
- XX int index
- X bool isOutside
- XXX int volume (ml)
- XXX int regular period (seconds)
- XXX int cooldown period (seconds)
No delimiters are needed.
*/
void setProfileInfo(String profileString)
{
    Profile *currentProfile = new Profile;
    int index;
#if SS_USE

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
#else
    index = profileString.substring(0, 2).toInt();
    currentProfile->isOutside = profileString.substring(2, 3).toInt() > 0;
    currentProfile->volume = profileString.substring(3, 6).toInt();
    currentProfile->regularPeriod = profileString.substring(6, 9).toInt();
    currentProfile->cooldownPeriod = profileString.substring(9, 12).toInt();
#endif
    Memory::setProfile(index, *currentProfile);
    delete currentProfile;
    return;
}

/*
Deserialize string fetched from BLE characteristic for setting a profile. The pattern used is "ssid,password".
*/
void setWiFiInfo(String WiFiString)
{
#if SS_USE
    char ssid[20], password[20];
    char delimiter = ',';
    std::istringstream iss(WiFiString.c_str());
    std::string numberStr;

    if (std::getline(iss, numberStr, delimiter))
    {
        strcpy(ssid, numberStr.c_str());
    }
    if (std::getline(iss, numberStr, delimiter))
    {
        strcpy(password, numberStr.c_str());
    }
#else
    String ssid, password;
    char delimiter = ',';
    int index = WiFiString.indexOf(delimiter);
    if (index >= 0)
    {
        ssid = WiFiString.substring(0, index);
        password = WiFiString.substring(index);
    }

#endif
    Memory::setWiFiConfig(ssid, password);
    return;
}

/*
Deserialize string fetched from BLE characteristic for setting a profile. The pattern used is:
- X bool inUseControl
- X int profileIndex
*/
void setControllerInfo(int controlIndex, String controllerString)
{
    bool inUseControl;
    int profileIndexControl;
#if SS_USE

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
#else
    inUseControl = controllerString.substring(0, 1).toInt() > 0;
    profileIndexControl = controllerString.substring(1, 2).toInt();
#endif
    if (!controllers[controlIndex].getInUse() && inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() + 1);
        controllers[controlIndex].setProfile(profileIndexControl);
        controllers[controlIndex].setInUse();
    }
    else if (controllers[controlIndex].getInUse() && !inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() - 1);
        controllers[controlIndex].setProfile(profileIndexControl);
        return;
    }
}
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic)
    {

        if (pCharacteristic->getUUID().toString() == C_WIFI_R_STATUS_UUID)
        {
            pCharacteristic->setValue(WiFiHandler::isConnected());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_STATUS_UUID[0])
        {
            pCharacteristic->setValue(controllers[0].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_STATUS_UUID[1])
        {
            pCharacteristic->setValue(controllers[1].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_STATUS_UUID[2])
        {
            pCharacteristic->setValue(controllers[2].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_STATUS_UUID[3])
        {
            pCharacteristic->setValue(controllers[3].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_PROFILE_UUID[0])
        {
            pCharacteristic->setValue(getProfileInfo(0));
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_PROFILE_UUID[1])
        {
            pCharacteristic->setValue(getProfileInfo(1));
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_PROFILE_UUID[2])
        {
            pCharacteristic->setValue(getProfileInfo(2));
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_PROFILE_UUID[3])
        {
            pCharacteristic->setValue(getProfileInfo(3));
        }

        // Serial.print(pCharacteristic->getUUID().toString().c_str());
        //  Serial.print(": onRead(), value: ");
        Serial.print(F("Value read:"));
        Serial.println((String)pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        if (pCharacteristic->getUUID().toString() == C_PROFILE_W_UUID)
        {
            setProfileInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_W_CONFIG_UUID)
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
        // Serial.print(pCharacteristic->getUUID().toString().c_str());
        // Serial.print(": onWrite(), value: ");
        Serial.print(F("Value Written: "));
        Serial.println(pCharacteristic->getValue().c_str());
    };
    /** Called before notification or indication is sent,
     *  the value can be changed here before sending if desired.
    void onNotify(NimBLECharacteristic *pCharacteristic)
    {
        Serial.println("Sending notification to clients");
    };
     */
};

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer)
    {
        Serial.println("Client connected");
        Serial.println("Multi-connect support: start advertising");
        NimBLEDevice::startAdvertising();
    };
    /** Alternative onConnect() method to extract details of the connection.
     *  See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
     */
    void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
    {
        Serial.print("Client address: ");
        Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
        /** We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments, try for 5x interval time for best results.
         */
        pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
    };
    void onDisconnect(NimBLEServer *pServer)
    {
        Serial.println("Client disconnected - start advertising");
        NimBLEDevice::startAdvertising();
    };
    void onMTUChange(uint16_t MTU, ble_gap_conn_desc *desc)
    {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest()
    {
        Serial.println("Server Passkey Request");
        /** This should return a random 6 digit number for security
         *  or make your own static passkey as done here.
         */
        return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key)
    {
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
        /** Return false if passkeys don't match. */
        uint32_t test_key = 123456;
        while (test_key)
        {
            if (test_key % 10 != pass_key % 10)
            {
                return false;
            }
            test_key /= 10;
            pass_key /= 10;
        }
        return true;
    };
};

static CharacteristicCallbacks chrCallbacks;

BLEHandler::BLEHandler() {}

void BLEHandler::setup()
{
    Serial.println(F("Starting NimBLE Server"));
    /** sets device name */
    NimBLEDevice::init("NimBLE-irrigacao");
    pServer = NimBLEDevice::createServer();

    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityPasskey(123456);

    pServer->setCallbacks(new ServerCallbacks());
    /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
    NimBLEDevice::setPower(9); /** +9db */
#endif

    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);//just connect
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); // use numeric comparison

    pWiFiService = pServer->createService(S_WIFI_UUID);
    pWiFiStatusCharacteristic = pWiFiService->createCharacteristic(
        C_WIFI_R_STATUS_UUID,
        NIMBLE_PROPERTY::READ);
    pWiFiConfigCharacteristic = pWiFiService->createCharacteristic(
        C_WIFI_W_CONFIG_UUID,
        // NIMBLE_PROPERTY::READ || // remove later
        NIMBLE_PROPERTY::WRITE_ENC);

    pWiFiStatusCharacteristic->setCallbacks(&chrCallbacks);
    pWiFiConfigCharacteristic->setCallbacks(&chrCallbacks);

    pProfileService = pServer->createService(S_PROFILE_UUID);
    pProfileWriteCharacteristic = pProfileService->createCharacteristic(
        C_PROFILE_W_UUID,
        NIMBLE_PROPERTY::WRITE_ENC);
    pProfileWriteCharacteristic->setCallbacks(&chrCallbacks);

    for (int i = 0; i < 4; i++)
    {
        pControllerService[i] = pServer->createService(S_CONTROL_UUID[i]);
        pControllerWriteCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_W_UUID[i], NIMBLE_PROPERTY::WRITE_ENC);
        pControllerWriteCharacteristic[i]->setCallbacks(&chrCallbacks);
        pControllerReadStatusCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_R_STATUS_UUID[i], NIMBLE_PROPERTY::READ);
        pControllerReadProfileCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_R_PROFILE_UUID[i], NIMBLE_PROPERTY::READ);
        pControllerReadStatusCharacteristic[i]->setCallbacks(&chrCallbacks);
        pControllerReadProfileCharacteristic[i]->setCallbacks(&chrCallbacks);
    }

    /** Start the services when finished creating all Characteristics and Descriptors */
    pWiFiService->start();
    pProfileService->start();
    for (int i = 0; i < 4; i++)
    {
        pControllerService[i]->start();
    }

    pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pWiFiService->getUUID());
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

    Serial.println(F("Advertising Started"));
}