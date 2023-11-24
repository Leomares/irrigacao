#define CONTROLLER_USE 0
#define API_USE 1
#define WIFI_USE 1
#define BLE_USE 0
#define MEMORY_USE 1
#define TIMER_USE 0

#include <Arduino.h>
#if MEMORY_USE
#include "Memory.h"
#endif
#if WIFI_USE
#include "WiFiHandler.h"
#endif
#if API_USE
#include "APIWrapper.h"
#endif
#if CONTROLLER_USE
#include "Controller.h"
#endif
#if BLE_USE
#include "BLEHandler.h"
#endif
#if API_USE
APIWrapper api;
#endif
#if BLE_USE
BLEHandler ble;
#endif

#if CONTROLLER_USE
const int sensorPins[4] = {34, 35, 36, 39}; // input only pins
const int pumpPins[4] = {32, 33, 25, 26};
Controller controllers[4] = {
    Controller(sensorPins[0], pumpPins[0]),
    Controller(sensorPins[1], pumpPins[1]),
    Controller(sensorPins[2], pumpPins[2]),
    Controller(sensorPins[3], pumpPins[3])};
#endif

#if TIMER_USE
hw_timer_t *timer = NULL;
uint64_t timerValue = 0;
hw_timer_t *timerAPI = NULL;

void IRAM_ATTR onTimer()
{
    timerValue++;
    return;
}
void IRAM_ATTR onTimerAPI()
{
#if API_USE
    api.getDataFromURL();
#endif
    Serial.println("API called");
    return;
}
#endif

void setup()
{
// put your setup code here, to run once:

// set default params
#if MEMORY_USE
    Memory::resetNVS();
    Memory::setDefaultWiFiConfig();
    Memory::setDefaultProfile();
#endif
// controller config
#if CONTROLLER_USE
    Controller::addNControllers(1);
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].setInUse();
    }
#endif
// timer config
#if TIMER_USE
    timer = timerBegin(0, 40000, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 2000, true);
    timerAPI = timerBegin(1, 40000, true);
    timerAttachInterrupt(timer, &onTimerAPI, true);
    timerAlarmWrite(timer, 2000 * 60 * 15, true); // 15 minutes
#endif
    // serial config
    Serial.begin(115200);

    // bluetooth config

#if TIMER_USE
    timerAlarmEnable(timer);
    timerAlarmEnable(timerAPI);
#endif
}

void loop()
{
// put your main code here, to run repeatedly:
#if CONTROLER_USE
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].control();
    }
#endif
    // delay(10 * 60 * 1000); // 10 minutes
    delay(1000); // 30 seconds
    Serial.println("Main loop");
}
