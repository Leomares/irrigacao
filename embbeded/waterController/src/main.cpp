// todo
// implement security on ble

#define CONTROLLER_USE 1
#define API_USE 1
#define WIFI_USE 1
#define BLE_USE 1
#define MEMORY_USE 1
#define TIMER_USE 1

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

#if CONTROLLER_USE
const int sensorPins[4] = {34, 35, 36, 39}; // input only pins
const int pumpPins[4] = {32, 33, 25, 26};
Controller controllers[4] = {
    Controller(0, sensorPins[0], pumpPins[0]),
    Controller(1, sensorPins[1], pumpPins[1]),
    Controller(2, sensorPins[2], pumpPins[2]),
    Controller(3, sensorPins[3], pumpPins[3])};
#endif

#if TIMER_USE
// volatile SemaphoreHandle_t timerSemaphore;
// portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *timer = NULL;
volatile static uint64_t isr_timerValue = 0;
uint64_t timerValue;
// hw_timer_t *timerAPI = NULL;
// volatile uint64_t isr_timerAPIValue = 0;
uint64_t timerAPIValue = 0;

void IRAM_ATTR onTimer()
{
    // portENTER_CRITICAL_ISR(&timerMux);
    isr_timerValue++;
    // portEXIT_CRITICAL_ISR(&timerMux);
    // xSemaphoreGiveFromISR(timerSemaphore, NULL);
    return;
}
#endif

void setup()
{
    // put your setup code here, to run once:
    delay(10000);
    // timerSemaphore = xSemaphoreCreateBinary();
#if WIFI_USE
    WiFiHandler wifi;
    WiFiHandler::connectWifi();
#endif

#if MEMORY_USE
    // Memory::resetNVS();
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
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true);
#endif

    // serial config
    Serial.begin(115200);

// bluetooth config
#if BLE_USE
    BLEHandler::setup();
#endif

#if TIMER_USE
    timerAlarmEnable(timer);
#endif
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(1000 * 60 * 5); // 5 minutes
    delay(30000);         // 30 seconds

    if (!WiFiHandler::isConnected())
    {
        Serial.println(F("Connecting to WiFi"));
        WiFiHandler::connectWifi();
    }
    // if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE)
    //{
    //     portENTER_CRITICAL_ISR(&timerMux);
    timerValue = isr_timerValue;
    delay(100);
    if (isr_timerValue > timerAPIValue)
    {
        timerAPIValue = isr_timerValue + 30;
        api.getDataFromURL();
    }
    float data = api.getData(120);
    Serial.printf("API data gathered from %d calls(%d): %1.4f\n", timerAPIValue / 30, timerValue, data);
#if CONTROLLER_USE
    for (int i = 0; i < Controller::getNControllers(); i++)
    {
        controllers[i].control();
    }
#endif
    if (!timerAlarmEnabled(timer))
    {
        timerAlarmEnable(timer);
    }
}
