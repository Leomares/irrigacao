#include <Arduino.h>
#include "Memory.h"
#include "APIWrapper.h"
#include "Controller.h"

class Controller
{
public:
    Controller(int soilMoisturePin, int pumpPin) : soilMoisturePin(soilMoisturePin), pumpPin(pumpPin)
    {
        inUse = false;
        onCooldown = false;
        pinMode(pumpPin, OUTPUT);
        digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    }

    void setProfile(int index)
    {
        struct Profile currentProfile;
        // read from preference namespace by index

        inUse = true;

        if (regularPeriod != 0)
        {
            nextEvent = regularPeriod;
        }
        else if (cooldownPeriod != 0)
        {
            nextEvent = cooldownPeriod;
        }
        else
        {
            nextEvent = 0;
        }

        return;
    }

    float getSoilMoisture()
    {
        int val = analogRead(soilMoisturePin);
        // conversion
        // float moisture
        return (float)val;
    }

    void turnOnWaterPump(int volume)
    {
        // volume [mL]
        int time_ms = (60 * volume) / this->pumpFlowRate;

        digitalWrite(pumpPin, true);
        delay(time_ms);
        digitalWrite(pumpPin, false);
        delay(10);
        return;
    }

    int calculateVolume(int soilMoistureLevel, int rainLevel)
    {
        int volume = 0;
        if (soilMoistureThreshold == 0)
        {
            // calculate volume = " timedVolume" - func(rainLevel)
        }
        else
        {
            // calculate volume = func2(soilMoistureThreshold, soilMoistureLevel, rainLevel)
        }
        return volume;
    }

    void UpdateNextEvent(uint_64_t timer)
    {
        if (regularPeriod != 0)
        {
            this->nextEvent = this->nextEvent + regularPeriod;
        }
        else if (cooldownPeriod != 0)
        {
            this->nextEvent = this->nextEvent + cooldownPeriod;
            this->onCooldown = true;
        }

        this->nextEvent = this->nextEvent % timer_max_value;

        return;
    }

    void control(float rainData)
    {
        if (!this->inUse)
        {
            return;
        }

        int soilMoistureLevel, rainLevel, pumpVolume;
        uint_64_t timer;

        // get_timer() from esp32 api

        if (this->soilMoistureThreshold != 0)
        {

            // if (this->nextEvent != 0 || this->onCooldown)
            //{
            //     return;
            // }

            if (nextEvent != 0)
            {
                if ((timer < nextEvent && (nextEvent - timer) < timer_max_cooldown) || timer - nextEvent > timer_overflow_th)
                {
                    return; // cooldown
                }
            }

            soilMoistureLevel = getSoilMoisture();
            pumpVolume = calculateVolume(soilMoistureLevel, rainData);

            if (pumpVolume != 0)
            {
                this->turnOnWaterPump(pumpVolume);

                this->UpdateNextEvent(timer);
            }
        }
        else
        {

            if (timer > nextEvent || nextEvent - timer > timer_overflow_th)
            {
                rainLevel = getRainFromAPI();

                pumpVolume = calculateVolume(0, rainLevel);

                turnOnWaterPump(pumpVolume);
                UpdateNextEvent(timer)
            }
        }
        return;
    }

    bool inUse;

private:
    // pin specific
    int soilMoisturePin;
    int pumpPin;
    // peripherals specific
    int soilMoistureThreshold;
    float pumpFlowRate; // flow rate [L/min]
    // profile
    int volume;
    int regularPeriod;  // between watering
    int cooldownPeriod; // without watering
    bool isOutside;
    // state
    int nextEvent;
    int onCooldown;
};
