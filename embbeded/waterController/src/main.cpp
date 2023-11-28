// todo
// trim code
// implement security on ble

#define API_USE 1

#include <Arduino.h>
#include "Memory.h"
#include "WiFiHandler.h"
#if API_USE
#include "APIWrapper.h"
APIWrapper api;
#endif
#include "Controller.h"
#include "BLEHandler.h"

const int sensorPins[4] = {34, 35, 36, 39}; // input only pins
const int pumpPins[4] = {32, 33, 25, 26};
Controller controllers[4] = {
    Controller(0, sensorPins[0], pumpPins[0]),
    Controller(1, sensorPins[1], pumpPins[1]),
    Controller(2, sensorPins[2], pumpPins[2]),
    Controller(3, sensorPins[3], pumpPins[3])};

hw_timer_t *timer = NULL;
volatile static uint64_t isr_timerValue = 0;
uint64_t timerValue;
static uint64_t timerAPIValue = 0;

void IRAM_ATTR onTimer()
{
    isr_timerValue++;
    return;
}

void setup()
{
    // put your setup code here, to run once:
    delay(10000);
    WiFiHandler wifi;
    WiFiHandler::connectWifi();
#if DEFAULT_MEMORY_CONFIG
    Memory::resetNVS();
    Memory::setDefaultWiFiConfig();
    Memory::setDefaultProfile();
#endif

    // controller config
    Controller::addNControllers(1);
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].setInUse();
    }

    // timer config
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true);

    // serial config
    Serial.begin(115200);

    // bluetooth config
    BLEHandler::setup();

    timerAlarmEnable(timer);
}

void loop()
{
    // put your main code here, to run repeatedly:
    // delay(1000 * 60 * 5); // 5 minutes
    delay(30000); // 30 seconds

    if (!WiFiHandler::isConnected())
    {
        // Serial.println(F("Connecting to WiFi"));
        WiFiHandler::connectWifi();
    }

    timerValue = isr_timerValue;
    delay(100);
    Serial.print(F("Current timer: "));
    Serial.println(timerValue);
    if (isr_timerValue > timerAPIValue)
    {
        timerAPIValue = isr_timerValue + 30; // debug
// timerAPIValue = isr_timerValue + 15*60;
#if API_USE
        api.getDataFromURL();
#endif
    }
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].control();
    }

    if (!timerAlarmEnabled(timer))
    {
        timerAlarmEnable(timer);
    }
}
