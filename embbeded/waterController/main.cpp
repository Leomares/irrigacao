// todo
// separate in files
// implement memory access as static method of a class
// put profile in struct
// initialize timers and pass pointer to control func
// update control
// - remove overflow handling
// - estimate quantity of water needed after rain based on volume (precip_mm + time elapsed + area of the plant)
// implement timer to update location data and rain data
// implement bluetooth handler

#include <iostream> //serial
// #include <EEPROM.h>
#include <Preferences.h> //storage in flash
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
using namespace std;

Preferences prefs;
DynamicJsonDocument doc(2048);

class Memory
{
public:
    Memory()
    {
        return;
    }

    static void setWiFiConfig(String ssid_, String password_)
    {
        prefs.begin("wifi", false);
        prefs.putString("ssid_string", ssid_);
        prefs.putString("password_string", password_);
        prefs.end();
        return;
    }

    static void getWiFiConfig(String &ssid_, String &password_)
    {
        prefs.begin("wifi", true);
        ssid_ = prefs.getString("ssid_string", "");
        password_ = prefs.getString("password_string", "");
        prefs.end();
        return;
    }

    static void setProfile(int index, bool isOutside, int volume, int regularPeriod = 10, int cooldownPeriod = 0)
    {
        String profile = "profile" + String(index,DEC);
        prefs.begin(profile.c_str(), false);
        prefs.putBool("valid", true);
        prefs.putBool("isOutside", isOutside);
        prefs.putInt("volume", volume);
        prefs.putInt("regularPeriod", regularPeriod);
        prefs.putInt("cooldownPeriod", cooldownPeriod);
        prefs.end();

        return;
    }

};

class WiFiHandler
{
public:
    WiFiHandler()
    {
        String ssid, password;
        Memory::getWiFiConfig(ssid, password);
        if (ssid == "" || password == "")
        {
            Serial.println("No wifi configuration.");
            // getSerialWifiConfig();
        }
        return;
    }

    WiFiHandler(String ssid, String password)
    {
        Memory::setWiFiConfig(ssid, password);
        return;
    }

    static void getSerialWifiConfig()
    {
        String ssid;
        String password;
        String response = "";

        Serial.print("Configure Wifi?");
        response = Serial.readString();
        if (response == "y")
        {
            // pass name and password to wifi namespace
            Serial.print("SSID to connect: ");
            ssid = Serial.readString();
            Serial.print("password: ");
            password = Serial.readString();
            Memory::setWiFiConfig(ssid, password);
        }
        return;
    }

    static void connectWifi()
    {
        // read wifi config
        String ssid, password;
        Memory::getWiFiConfig(ssid, password);
        // initialize wifi
        if (ssid != "")
        {
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid, password);
            Serial.println("Setting up Wifi.");

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
            }
            else
            {
                Serial.println("Wifi timeout.");
            }
        }
        else
        {
            Serial.println("No Wifi config found.");
        }
        return;
    }

    static bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }
};

// https://www.weatherapi.com/docs/
// https://ipstack.com/
// https://wokwi.com/projects/371565043567756289
DynamicJsonDocument doc(1024);
class APIWrapper
{
public:
    APIWrapper(char *url,int interval) : url(url)
    {
    precip_mm = 0.0;
    current_timestamp = 0;
    accumulated_precip = 0;
    accumulated_timestamp = 0;
    this->interval = interval;
    }

    void getDataFromURL()
    {
        if (WiFiHandler::isConnected())
        {
            Serial.println("Getting current data...");

            http.begin(url);
            int httpCode = http.GET();
            Serial.print("HTTP Code: ");
            Serial.println(httpCode);
            if (httpCode > 0)
            {
                DeserializationError error = deserializeJson(doc, http.getString());

                if (error)
                {
                    Serial.print(F("deserializeJson failed: "));
                    Serial.println(error.f_str());
                    http.end();
                    delay(2500);
                    return;
                }
            }
            Serial.println("Successful API call");
            http.end();

            // data collection
            strlcpy(last_updated, doc["current"]["last_updated"] | "YYYY-MM-DD HH HH:MM", 16);
            precip_mm = doc["current"]["precip_mm"] | 0;
            current_timestamp ++;
            if (current_timestamp >= 24*60/interval) {
              current_timestamp = 0;
              accumulated_precip = 0;
              accumulated_timestamp = 0;
            }
            if (precip_mm > 0) {
              accumulated_precip += precip_mm;
              accumulated_timestamp ++;
            }
        }
        else
        {
            Serial.println("WiFi not connected.");
        }
        return;
    }

    void getData(&accumulated,&time){
      accumulated = accumulated_precip;
      time = accumulated_timestamp;
      return;
    }

private:
    char *url;
    int interval;
    char *last_updated;
    float precip_mm;
    int current_timestamp;
    float accumulated_precip;
    int accumulated_timestamp;
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

        prefs.begin("profile" + to_string(index), true);
        validProfile = prefs.getBool("valid", false);
        if (!validProfile)
            return;
        isOutside = prefs.getBool("isOutside", false);
        volume = prefs.getInt("volume", 0);
        regularPeriod = prefs.getInt("regularPeriod", 0);
        cooldownPeriod = prefs.getInt("cooldownPeriod", 0);
        prefs.end();

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

APIWrapper api('https://api.chucknorris.io/jokes/random',15);

int n_controllers = 0;
int sensorPins[4] = {-1, -1, -1, -1};
int pumpPins[4] = {-1, -1, -1, -1};
Plant *profiles[4] = {};

void setup()
{
    // put your setup code here, to run once:

    Serial.begin(115200);
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

    // set bluetooth
}

void loop()
{
    // put your main code here, to run repeatedly:
    api.getDataFromURL("value");
    Serial.println(api.getData());
    if (WiFi.status() != WL_CONNECTED)
    {
        api.connectWifi();
    }
    api.getDataFromURL();
    char *rainData = api.getRainData("value");
    for (int i = 0; i < n_controllers; i++)
    {
        profiles[i].control(rainData);
    }
    // delay(10 * 60 * 1000); // 10 minutes
    delay(10 * 1000);
}
