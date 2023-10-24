// todo
// implement wrapper
// update control
// initialize timers and pass pointer to control func
// we dont need to deal with timer cycle, its only updated each second

#include <iostream>
// #include <EEPROM.h>
#include <Preferences.h> //storage in flash
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
using namespace std;

// #define EEPROM_MAX_SIZE 512; // Adjust the size as needed
// #define PROFILES_SIZE 5 * 4;
//  #define SSID_ADDRESS 0           // Address to store SSID
//  #define PASS_ADDRESS 64          // Address to store password
//  #define MAX_CREDENTIAL_LENGTH 64 // Adjust the length as needed

#define timer_max_cooldown 0x100000;          // 2^20
#define timer_overflow_th 0x1000000000000000; // 2^60
#define timer_max_value (0x1 << 64);

// https://www.weatherapi.com/docs/
class APIWrapper
{
public:
    void setWifiConfig()
    {
        // set name and password name
    }

    void getDataFromURL(char *url)
    {
    }

    void parseData()
    {
    }

    void updateData()
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
        inUse = false;
        onCooldown = false;
        pinMode(pumpPin, OUTPUT);
        digitalWrite(pumpPin, LOW); // Ensure the pump is initially turned off
    }

    void setProfile(int index)
    {
        // read from preference namespace by index
        bool validProfile;

        preferences.begin("profile" + to_string(index), true);
        validProfile = preferences.getBool("valid", false);
        if (!validProfile)
            return;
        isOutside = preferences.getBool("isOutside", false);
        volume = preferences.getInt("volume", 0);
        regularPeriod = preferences.getInt("regularPeriod", 0);
        cooldownPeriod = preferences.getInt("cooldownPeriod", 0);
        preferences.end();

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
        float moisture;
        int val = analogRead(soilMoisturePin);
        // conversion
        // float moisture
        return moisture;
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

void writeProfile(int index, bool isOutside, int volume, int regularPeriod = 10, int cooldownPeriod = 0)
{
    preferences.begin("profile" + to_string(index), false);
    preferences.putBool("valid", true);
    preferences.putBool("isOutside", isOutside);
    preferences.putInt("volume", volume);
    preferences.putInt("regularPeriod", regularPeriod);
    preferences.putInt("cooldownPeriod", cooldownPeriod);
    preferences.end();

    return;
}

void changeWifiConfig(string ssid_, string password_)
{
    preferences.begin("wifi", false);
    preferences.putString("ssid_string", ssid_);
    preferences.putString("password_string", password_);
    preferences.end();

    return;
}

Preferences preferences;

char *ssid;
char *password;
bool connected;
string response = "";
DynamicJsonDocument doc(2048);

APIWrapper api;

int n_controllers = 0;
int sensorPins[4] = {-1, -1, -1, -1};
int pumpPins[4] = {-1, -1, -1, -1};
Plant *profiles[4] = {};

void setup()
{
    Serial.begin(115200);
    // put your setup code here, to run once:
    Serial.print('Setting up profiles');
    for (int i = 0; i < 4; i++)
    {
        // update to read the first variable of a profile on flash?
        // create a namespace to load the last profile used on these pins
        if (sensorPins[i] < 0 || pumpPins[i] < 0)
        {
            break;
        }
        n_controllers++;
        profiles[i] = new Plant(sensorPins[i], pumpPins[i]);
        profiles[i]->setProfile(i);
    }

    Serial.print("Configure Wifi?");
    response = Serial.readString();
    if (response == "y")
    {
        // pass name and password to wifi namespace
    }

    // read wifi config
    preferences.begin("wifi", false);
    ssid = preferences.getString("ssid_string", "default");
    password = preferences.getString("password_string", "default");
    preferences.end();
    // initialize wifi
    if (trcmp(ssid, "default") != 0)
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        Serial.println("Setting up Wifi");

        // Wait for connection
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 30000)
        {
            delay(500);
            timeout += 500;
            Serial.print(".");
        }
        if (timeout < 30000)
        {
            Serial.print("WiFi connected with IP: ");
            Serial.println(WiFi.localIP());
            connected = true;
        }
        else
        {
            Serial.print("Wifi timeout. Weather API won't be used.");
            connected = false;
        }
    }
    else
    {
        Serial.print("No Wifi config found. Weather API won't be used.");
        connected = false;
    }

    // set bluetooth
}

void loop()
{
    // put your main code here, to run repeatedly:
    float rainData = api.getRainData();
    for (int i = 0; i < n_controllers; i++)
    {
        profiles[i].control(rainData);
    }
    // Delay(10 * 60 * 1000); // 10 minutes
    Delay(10 * 1000);
}