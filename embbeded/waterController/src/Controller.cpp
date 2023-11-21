#include <Arduino.h>
#include "Memory.h"
#include "APIWrapper.h"
#include "Controller.h"

Controller::Controller(int soilMoisturePin, int pumpPin) : soilMoisturePin(soilMoisturePin), pumpPin(pumpPin)
{
    soilMoistureThreshold = 600; // need to measure
    pumpFlowRate = 2 * 0.5;      // maximum of 2 L/min

    inUse = false;
    profileIndex = -1;

    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
}

bool Controller::getInUse()
{
    return inUse;
}

void Controller::setInUse()
{
    updateNextEvent();
    inUse = true;
    return;
}

void Controller::setProfile(int index)
{
    profileIndex = index;
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
    delay(10);
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

void Controller::control(APIWrapper api)
{
    if (!inUse || nextEvent > timerValue)
    {
        return;
    }

    int rangeAPI;
    float rainLevel, volumeNeeded;

    Profile currentProfile = Memory::getProfile(profileIndex);
    float soilMoistureLevel = getSoilMoistureData();

    if (currentProfile.regularPeriod != 0)
    {
        rangeAPI = currentProfile.regularPeriod;
        rainLevel = api.getData(rangeAPI);
    }
    else if (currentProfile.cooldownPeriod != 0)
    {
        rangeAPI = currentProfile.regularPeriod;
        rainLevel = api.getData(rangeAPI);
    }
    else
    {
        rainLevel = 0;
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