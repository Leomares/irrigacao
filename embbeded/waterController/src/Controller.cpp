#include <Arduino.h>
#include "Memory.h"
#include "APIWrapper.h"
#include "Controller.h"

extern APIWrapper api;

Controller::Controller(int controllerIndex, int soilMoisturePin, int pumpPin) : controllerIndex(controllerIndex), soilMoisturePin(soilMoisturePin), pumpPin(pumpPin)
{
    soilMoistureThreshold = 1000; // need to measure
    pumpFlowRate = 2 * 0.5;       // maximum of 2 L/min

    inUse = false;
    profileIndex = -1;
}

bool Controller::getInUse()
{
    return inUse;
}

void Controller::setInUse()
{
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    profileIndex = Memory::getLastProfile(controllerIndex);
    updateNextEvent();
    inUse = true;
    return;
}

int Controller::getProfileIndex()
{
    return profileIndex;
}

void Controller::setProfile(int index)
{
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    Memory::setLastProfile(controllerIndex, index);
    profileIndex = index;
    inUse = false;
    return;
}

float Controller::getSoilMoistureData()
{
    uint16_t measure = analogRead(soilMoisturePin);
    return (float)measure;
}

void Controller::turnOnWaterPump(float volume)
{
    // volume [mL]
    int time_ms = (60 * volume) / this->pumpFlowRate;
    Serial.printf("Turning on water pump for %d mseconds\n", time_ms);
    pinMode(LED_PUMP, OUTPUT);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(pumpPin, true);
    delay(time_ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(pumpPin, false);
    delay(100);
    return;
}

float Controller::calculateVolume(float soilMoistureLevel, float rainLevel)
{
    int volumeNeeded = 0;

    struct Profile currentProfile;
    currentProfile = Memory::getProfile(profileIndex);

    // area of the plant in m2
    float estimated_area = 0.25;
    Serial.printf("Calculate volume: %f,%f,%d\n", soilMoistureLevel, rainLevel, currentProfile.volume);
    if (currentProfile.cooldownPeriod > 0)
    {
        volumeNeeded = (float)currentProfile.volume - rainLevel * estimated_area * 1000;
        if (volumeNeeded < 0)
        {
            volumeNeeded = 0;
        }
    }
    else if (currentProfile.regularPeriod > 0 && (int)soilMoistureLevel < soilMoistureThreshold)
    {
        volumeNeeded = 0;
    }
    else
    {
        volumeNeeded = currentProfile.volume;
    }
    return volumeNeeded;
}

void Controller::updateNextEvent()
{
    struct Profile currentProfile;
    // read from preference namespace by index
    currentProfile = Memory::getProfile(profileIndex);

    if (currentProfile.regularPeriod != 0)
    {
        nextEvent = timerValue + currentProfile.regularPeriod;
    }
    else if (currentProfile.cooldownPeriod != 0)
    {
        nextEvent = timerValue + currentProfile.cooldownPeriod;
    }
    else
    {
        nextEvent = 0;
    }

    return;
}

void Controller::control()
{
    // Serial.println(F("Controller control engaged"));
    if (!inUse || nextEvent > timerValue)
    {
        Serial.println(F("Controller not in use"));
        return;
    }

    int rangeAPI;
    float rainLevel = 0.0;
    float volumeNeeded;

    Profile currentProfile = Memory::getProfile(profileIndex);
    float soilMoistureLevel = getSoilMoistureData();

    if (currentProfile.regularPeriod != 0)
    {
        rangeAPI = currentProfile.regularPeriod;
        rainLevel = api.getData(rangeAPI);
    }
    else if (currentProfile.cooldownPeriod != 0)
    {
        rangeAPI = currentProfile.cooldownPeriod;
        rainLevel = api.getData(rangeAPI);
    }

    volumeNeeded = calculateVolume(soilMoistureLevel, rainLevel);
    Serial.printf("Controller %d needs %f ml\n", controllerIndex, volumeNeeded);
    turnOnWaterPump(volumeNeeded);

    updateNextEvent();
    return;
}

void Controller::addNControllers(int n)
{
    if (n > 4)
    {
        Serial.println(F("Too many simultaneous controllers"));
        return;
    }
    nControllers = n;
    return;
}

int Controller::getNControllers()
{
    return nControllers;
}

int Controller::nControllers = 0;
