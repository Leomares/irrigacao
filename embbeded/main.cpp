// int N_profiles = 1;

class APIWrapper
{

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

    auto getRainData()
    {
    }

private:
    // wifi config
    // data

}

class Plant
{

public:
    Plant(int soilMoisturePin, int pumpPin) : soilMoisturePin(soilMoisturePin), pumpPin(pumpPin)
    {
        pinMode(pumpPin, OUTPUT);
        digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    }

    void setProfile()
    {
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
        int volume;
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
        if (timedPeriod != 0)
        {
            nextEvent = nextEvent + timedPeriod;
        }
        else if (cooldownPeriod != 0)
        {
            nextEvent = nextEvent + cooldownPeriod;
        }
        return;
    }

    void control()
    {
        int soilMoistureLevel, rainLevel, pumpVolume;
        uint_64_t timer;

        // get_timer() from esp32 api
        if (soilMoistureThreshold != 0)
        {

            if (nextEvent != 0)
            {
                If((timer < nextEvent && (nextEvent - timer) < timer_max_cooldown) || timer - nextEvent > timer_overflow_th)
                {
                    Return // cooldown
                }
            }

            soilMoistureLevel = getSoilMoisture();
            rainLevel = getRainFromAPI();

            pumpVolume = calculateVolume(soilMoistureLevel, rainLevel)

                if (pumpVolume != 0)
            {
                turnOnWaterPump(pumpVolume)

                    UpdateNextEvent(timer)
            }
        }
        else
        {

            if (timer > nextEvent or nextEvent - timer > timer_overflow_th)
            {
                rainLevel = getRainFromAPI();

                pumpVolume = calculateVolume(0, rainLevel)

                    turnOnWaterPump(pumpVolume);
                UpdateNextEvent(timer)
            }
        }
        Return;
    }

private:
    bool inUse;
    int soilMoisturePin;
    int pumpPin;
    int soilMoistureThreshold;
    float pumpFlowRate; // flow rate [L/min]
    int timedVolume;
    int timedPeriod;
    int cooldownPeriod;                         // could use "timedPeriod" for both
    int timer_max_cooldown = 0x100000;          // 2^20
    int timer_overflow_th = 0x1000000000000000; // 2^60
};

Plant profiles[4];

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
        profiles[i].control();
    }
    Delay(10 * 60 * 1000); // 10 minutes
}