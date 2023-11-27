#include <Arduino.h>
#include "Memory.h"
#include "APIWrapper.h"
#include "Controller.h"

extern APIWrapper api;

Controller::Controller(int controllerIndex, int soilMoisturePin, int pumpPin) : controllerIndex(controllerIndex), soilMoisturePin(soilMoisturePin), pumpPin(pumpPin)
{
    soilMoistureThreshold = 2000; // need to measure
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
    profileIndex = Memory::getLastProfile(controllerIndex);
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    updateNextEvent();
    inUse = true;
    return;
}

void Controller::setProfile(int index)
{
    Memory::setLastProfile(controllerIndex, index);
    inUse = false;
    return;
}

float Controller::getSoilMoistureData()
{
    int measure = analogRead(soilMoisturePin);
    return (float)measure;
}

void Controller::turnOnWaterPump(float volume)
{
    // volume [mL]
    int time_ms = (60 * volume) / this->pumpFlowRate;

    digitalWrite(pumpPin, true);
    delay(time_ms);
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

    if (soilMoistureThreshold == 0)
    {
        volumeNeeded = min(int((float)currentProfile.volume - rainLevel * estimated_area * 1000), 0);
    }
    else if (getSoilMoistureData() < soilMoistureThreshold)
    {
        volumeNeeded = currentProfile.volume;
    }
    else
    {
        volumeNeeded = 0;
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
    Serial.println("Controller control engaged");
    if (!inUse || nextEvent > timerValue)
    {
        Serial.println("Controller not in use");
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

    turnOnWaterPump(volumeNeeded);
    updateNextEvent();
    return;
}

void Controller::addNControllers(int n)
{
    if (n > 4)
    {
        Serial.println("Too many simultaneous controllers");
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
