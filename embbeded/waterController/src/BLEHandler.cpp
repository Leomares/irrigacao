#include <Arduino.h>
#include "Memory.h"
#include "APIWrapper.h"
#include "Controller.h"
#include "BLEHandler.h"


// https://registry.platformio.org/libraries/h2zero/NimBLE-Arduino/examples/NimBLE_Server/NimBLE_Server.ino



/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */

/** Handler class for characteristic actions */
// test
class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic){
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic* pCharacteristic) {
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onWrite(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
    };
    /** Called before notification or indication is sent,
     *  the value can be changed here before sending if desired.
     */
    void onNotify(NimBLECharacteristic* pCharacteristic) {
        Serial.println("Sending notification to clients");
    };
};

static CharacteristicCallbacks chrCallbacks;


BLEHandler::BLEHandler() {
    
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

    pWifiService = pServer->createService("WIFI"); // put standard UUID later
    pWifiWriteCharacteristic = pWifiService->createCharacteristic(
                                               "WRIT",
                                               NIMBLE_PROPERTY::WRITE
                                              );

    pWifiReadCharacteristic = pWifiService->createCharacteristic(
                                               "READ",
                                               NIMBLE_PROPERTY::READ
                                              );

    pWifiWriteCharacteristic->setCallbacks(&chrCallbacks);
    pWifiReadCharacteristic->setCallbacks(&chrCallbacks);

    pProfileService = pServer->createService("PROF"); // put standard UUID later
    pProfileWriteCharacteristic = pProfileService->createCharacteristic(
                                               "WRIT",
                                               NIMBLE_PROPERTY::WRITE
                                              );

    pProfileReadCharacteristic = pProfileService->createCharacteristic(
                                               "READ",
                                               NIMBLE_PROPERTY::READ
                                              );

    pProfileWriteCharacteristic->setCallbacks(&chrCallbacks);
    pProfileReadCharacteristic->setCallbacks(&chrCallbacks);


    /** Start the services when finished creating all Characteristics and Descriptors */
    pWifiService->start();
    pProfileService->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pWifiService->getUUID());
    pAdvertising->addServiceUUID(pProfileService->getUUID());
    /** If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("Advertising Started");
}