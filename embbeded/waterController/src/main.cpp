

// measure soil humidity
// initialize timer to handle the api calls
//  update control
//  - remove overflow handling
//  - estimate quantity of water needed after rain based on volume (precip_mm + time elapsed + area of the plant)
//  implement timer to update location data and rain data
//  implement bluetooth handler

#include "Memory.h"
#include "WiFiHandler.h"
#include "APIWrapper.h"
#include "Controller.h"
#include "secrets.h"

APIWrapper api;

const int sensorPins[4] = {34, 35, 36, 39}; // input only pins
const int pumpPins[4] = {32, 33, 25, 26};
Controller controllers[4] = {
    Controller(sensorPins[0], pumpPins[0]),
    Controller(sensorPins[1], pumpPins[1]),
    Controller(sensorPins[2], pumpPins[2]),
    Controller(sensorPins[3], pumpPins[3])};

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
    api.getDataFromURL();
    return;
}

void setup()
{
    // put your setup code here, to run once:

    // set default params
    Memory::setWiFiConfig(home_ssid, home_password);
    Memory::setDefaultProfile();

    // controller config
    Controller::addNControllers(1);
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].setInUse();
    }

    // timer config
    timer = timerBegin(0, 40000, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 2000, true);
    timerAPI = timerBegin(1, 40000, true);
    timerAttachInterrupt(timer, &onTimerAPI, true);
    timerAlarmWrite(timer, 2000 * 60 * 15, true); // 15 minutes

    // serial config
    Serial.begin(115200);

    // bluetooth config

    timerAlarmEnable(timer);
    timerAlarmEnable(timerAPI);
}

void loop()
{
    // put your main code here, to run repeatedly:
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].control(api);
    }
    // delay(10 * 60 * 1000); // 10 minutes
    delay(30 * 1000); // 30 seconds
}
