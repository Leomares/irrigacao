#ifndef CONTROLLER_H
#define CONTROLLER_H

extern hw_timer_t *timerControl;

class Controller
{
public:
    Controller(int soilMoisturePin, int pumpPin);

    bool getInUse();

    /*enables the controller if a profile is avaliable.
     */
    void setInUse();

    void setProfile(int index);

    float getSoilMoistureData();

    void turnOnWaterPump(float volume);

    float calculateVolume(float soilMoistureLevel, float rainLevel);

    void updateNextEvent();

    /* Controls the digital pin of the water pump. If these pins are in use - refer to function setInUse() - and the timer value has passed the next event scheduled, the function estimates the amount of water needed based on the moisture measurement or/and the rain history.
     */
    void control(APIWrapper api);

private:
    // pin specific
    int soilMoisturePin;
    int pumpPin;
    // peripherals specific
    int soilMoistureThreshold;
    float pumpFlowRate; // flow rate [L/min]
    // profile
    int profileIndex;
    // state of the control
    bool inUse;
    uint64_t nextEvent;
};

#endif