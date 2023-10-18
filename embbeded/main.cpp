// int N_profiles = 1;

#define timer_max_cooldown 0x100000;          // 2^20
#define timer_overflow_th 0x1000000000000000; // 2^60
#define timer_max_value (0x1 << 64);

class APIWrapper
{
public:
    auto setWifiConfig()
    {
        // set name and password name
    }

    auto getDataFromURL(char *url)
    {
    }

    auto parseData()
    {
    }

    auto updateData()
    {
    }

    int getRainData()
    {
    }

private:
    // wifi config
    // data
};

class Plant
{
public:
    Plant(int soilMoisturePin, int pumpPin) : soilMoisturePin(soilMoisturePin), pumpPin(pumpPin)
    {
        this->inUse = false;
        this->onCooldown = false;
        pinMode(pumpPin, OUTPUT);
        digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    }

    void setProfile(int timedVolume = 10, int regularPeriod = 0, int cooldownPeriod = 0)
    {
        this->inUse = true;
        this->timedVolume = timedVolume;
        this->regularPeriod = regularPeriod;
        this->cooldownPeriod = cooldownPeriod;
        if (regularPeriod != 0)
        {
            this->nextEvent = regularPeriod;
        }
        else if (cooldownPeriod)
        {
            this->nextEvent = cooldownPeriod;
        }
        else
        {
            this->nextEvent = 0;
        }
    }

    float getSoilMoisture()
    {
        float moisture;
        int val = analogRead(soilMoisturePin);
        // conversion
        // float moisture
        return moisture;
    }

    void turnOnWaterPump(int volume)
    {
        // volume [mL]
        int time_ms = (60 * volume) / pumpFlowRate;

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

        this->nextEvent = this->nextEvent % ;

        return;
    }

    void control(APIWrapper api)
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
            rainLevel = api.getRainData();
            pumpVolume = calculateVolume(soilMoistureLevel, rainLevel);

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

                pumpVolume = calculateVolume(0, rainLevel)

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
    int timedVolume;
    int regularPeriod;  // between watering
    int cooldownPeriod; // without watering
    // state
    int nextEvent;
    int onCooldown;
};

Plant profiles[4];
APIWrapper api;

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    // set bluetooth
    // setup timer
    // ler arquivo e puxar config se tiver
    // inicia wifi
    // seta perfil(is)/ instancia Plant
}

void loop()
{
    // put your main code here, to run repeatedly:
    for (int i = 0; i < 4; i++)
    {
        profiles[i].control(api);
    }
    // Delay(10 * 60 * 1000); // 10 minutes
    Delay(10 * 1000);
}