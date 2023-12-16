#define TOKEN_ENABLE 0

#include <Arduino.h>
#include <NimBLEDevice.h>
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
static NimBLECharacteristic *pWiFiConfigSCharacteristic;
static NimBLECharacteristic *pWiFiConfigPCharacteristic;
const char *C_WIFI_R_W_CONFIG_SSID_UUID = "ab8caa74-9983-11ee-b9d1-0242ac120002";
const char *C_WIFI_W_CONFIG_PSSD_UUID = "2a387bdc-8a3e-11ee-b9d1-0242ac120002";

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

// characteristic to read and write whether a controller is enable or not.
static NimBLECharacteristic *pControllerStatusCharacteristic[4];
const char *C_CONTROL_R_W_STATUS_UUID[4] = {
    "8f904b86-8a3e-11ee-b9d1-0242ac120002",
    "c6a16c68-8a3e-11ee-b9d1-0242ac120002",
    "c6a1700a-8a3e-11ee-b9d1-0242ac120002",
    "c6a1719a-8a3e-11ee-b9d1-0242ac120002"};

// characteristic to read the current profile of a controller.
static NimBLECharacteristic *pControllerProfileCharacteristic[4];
const char *C_CONTROL_R_W_PROFILE_UUID[4] = {
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
    Serial.println("Profile " + String(profileIndex) + " from controller " + String(controllerIndex));
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
- X bool isOutside
- XXX int volume (ml)
- XXX int regular period (seconds)
- XXX int cooldown period (seconds)
No delimiters are needed.
*/
void setProfileInfo(int controllerIndex, String profileString)
{
    Profile *currentProfile = new Profile;

    currentProfile->isOutside = profileString.substring(0, 1).toInt() > 0;
    currentProfile->volume = profileString.substring(1, 4).toInt();
    currentProfile->regularPeriod = profileString.substring(4, 7).toInt();
    currentProfile->cooldownPeriod = profileString.substring(7, 10).toInt();

    Memory::setProfile(controllerIndex, *currentProfile);
    controllers[controllerIndex].setProfile(controllerIndex);
    delete currentProfile;
    return;
}

/*
Deserialize string fetched from BLE characteristic for setting a profile. The pattern used is "ssid,password".
*/
void setWiFiSSIDInfo(String WiFiString)
{
    String ssid, password;
    Memory::getWiFiConfig(&ssid, &password);
    // char delimiter = ',';
    // int index = WiFiString.indexOf(delimiter);
    // if (index >= 0)
    //{
    //     ssid = WiFiString.substring(0, index);
    //     password = WiFiString.substring(index);
    // }

    Memory::setWiFiConfig(WiFiString, password);
    return;
}

void setWiFiPasswordInfo(String WiFiString)
{
    String ssid, password;
    Memory::getWiFiConfig(&ssid, &password);
    // char delimiter = ',';
    // int index = WiFiString.indexOf(delimiter);
    // if (index >= 0)
    //{
    //     ssid = WiFiString.substring(0, index);
    //     password = WiFiString.substring(index);
    // }

    Memory::setWiFiConfig(ssid, WiFiString);
    return;
}

/*
Deserialize string fetched from BLE characteristic for setting a profile. The pattern used is:
- X bool inUseControl
*/
void setControllerInfo(int controllerIndex, String controllerString)
{
    bool inUseControl = controllerString.toInt() > 0;

    if (!controllers[controllerIndex].getInUse() && inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() + 1);
        controllers[controllerIndex].setInUse();
    }
    else if (controllers[controllerIndex].getInUse() && !inUseControl)
    {
        Controller::addNControllers(Controller::getNControllers() - 1);
        int currentIndex = controllers[controllerIndex].getProfileIndex();
        controllers[controllerIndex].setProfile(currentIndex);
    }
    return;
}

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic)
    {

        if (pCharacteristic->getUUID().toString() == C_WIFI_R_STATUS_UUID)
        {
            pCharacteristic->setValue((String)WiFiHandler::isConnected());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_R_W_CONFIG_SSID_UUID)
        {
            String ssid, password;
            Memory::getWiFiConfig(&ssid, &password);
            pCharacteristic->setValue(ssid);
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[0])
        {
            pCharacteristic->setValue(controllers[0].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[1])
        {
            pCharacteristic->setValue(controllers[1].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[2])
        {
            pCharacteristic->setValue(controllers[2].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[3])
        {
            pCharacteristic->setValue(controllers[3].getInUse());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[0])
        {
            pCharacteristic->setValue(getProfileInfo(0));
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[1])
        {
            pCharacteristic->setValue(getProfileInfo(1));
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[2])
        {
            pCharacteristic->setValue(getProfileInfo(2));
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[3])
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
            String data = pCharacteristic->getValue();
            setProfileInfo(data.substring(0, 1).toInt(), data.substring(1));
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_R_W_CONFIG_SSID_UUID)
        {
            setWiFiSSIDInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_WIFI_W_CONFIG_PSSD_UUID)
        {
            setWiFiPasswordInfo(pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[0])
        {
            setControllerInfo(0, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[1])
        {
            setControllerInfo(1, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[2])
        {
            setControllerInfo(2, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_STATUS_UUID[3])
        {
            setControllerInfo(3, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[0])
        {
            setProfileInfo(0, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[1])
        {
            setProfileInfo(1, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[2])
        {
            setProfileInfo(2, pCharacteristic->getValue());
        }
        else if (pCharacteristic->getUUID().toString() == C_CONTROL_R_W_PROFILE_UUID[3])
        {
            setProfileInfo(3, pCharacteristic->getValue());
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
        NimBLEDevice::startSecurity(desc->conn_handle);
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
#if TOKEN_ENABLE
    uint32_t onPassKeyRequest()
    {
        Serial.println("Server Passkey Request");
        /** This should return a random 6 digit number for security
         *  or make your own static passkey as done here.
         */
        return 654321;
    };

    void onAuthenticationComplete(ble_gap_conn_desc *desc)
    {
        /** Check that encryption was successful, if not we disconnect the client */
        if (!desc->sec_state.encrypted)
        {
            NimBLEDevice::getServer()->disconnect(desc->conn_handle);
            Serial.println("Encrypt connection failed - disconnecting client");
            return;
        }
        Serial.println("Starting BLE work!");
    };
#endif
};

static CharacteristicCallbacks chrCallbacks;
static ServerCallbacks *svrCallbacks = new ServerCallbacks();

BLEHandler::BLEHandler() {}

void BLEHandler::setup()
{
    Serial.println(F("Starting NimBLE Server"));
    /** sets device name */
    NimBLEDevice::init("NimBLE-irrigacao");
    pServer = NimBLEDevice::createServer();
#if TOKEN_ENABLE
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityPasskey(654321);
#endif
    pServer->setCallbacks(svrCallbacks);
    /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
    NimBLEDevice::setPower(9); /** +9db */
#endif

#if TOKEN_ENABLE
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // use passkey
#else
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT); // just connect
#endif
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); // use numeric comparison

    pWiFiService = pServer->createService(S_WIFI_UUID);
    pWiFiStatusCharacteristic = pWiFiService->createCharacteristic(
        C_WIFI_R_STATUS_UUID,
        NIMBLE_PROPERTY::READ);
    pWiFiConfigSCharacteristic = pWiFiService->createCharacteristic(
        C_WIFI_R_W_CONFIG_SSID_UUID,
        // NIMBLE_PROPERTY::READ || // remove later
        NIMBLE_PROPERTY::WRITE);
    pWiFiConfigPCharacteristic = pWiFiService->createCharacteristic(
        C_WIFI_W_CONFIG_PSSD_UUID,
        // NIMBLE_PROPERTY::READ || // remove later
        NIMBLE_PROPERTY::WRITE);

    pWiFiStatusCharacteristic->setCallbacks(&chrCallbacks);
    pWiFiConfigSCharacteristic->setCallbacks(&chrCallbacks);
    pWiFiConfigPCharacteristic->setCallbacks(&chrCallbacks);

    pProfileService = pServer->createService(S_PROFILE_UUID);
    pProfileWriteCharacteristic = pProfileService->createCharacteristic(
        C_PROFILE_W_UUID,
        NIMBLE_PROPERTY::WRITE);
    pProfileWriteCharacteristic->setCallbacks(&chrCallbacks);

    for (int i = 0; i < 4; i++)
    {
        pControllerService[i] = pServer->createService(S_CONTROL_UUID[i]);
        pControllerStatusCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_R_W_STATUS_UUID[i], NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
        pControllerStatusCharacteristic[i]->setCallbacks(&chrCallbacks);
        pControllerProfileCharacteristic[i] = pControllerService[i]->createCharacteristic(C_CONTROL_R_W_PROFILE_UUID[i], NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
        pControllerProfileCharacteristic[i]->setCallbacks(&chrCallbacks);
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

    pinMode(LED_BLE_ACTION, OUTPUT);
    delay(100);
    digitalWrite(LED_BLE_ACTION, HIGH);
    delay(100);
    digitalWrite(LED_BLE_ACTION, LOW);
}