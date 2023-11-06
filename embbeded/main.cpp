// todo
// implement memory access as static method of a class
// initialize timers and pass pointer to control func
// update control and remove overflow handling
// implement timer to update location data and rain data
// implement bluetooth handler

#include <iostream> //serial
// #include <EEPROM.h>
#include <Preferences.h> //storage in flash
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
using namespace std;

Preferences preferences;

class Memory
{
public:
    Memory()
    {
        return;
    }

    static void setWiFiConfig(String ssid_, String password_)
    {
        preferences.begin("wifi", false);
        preferences.putString("ssid_string", ssid_);
        preferences.putString("password_string", password_);
        preferences.end();
        return;
    }

    static void getWiFiConfig(String *ssid_, String *password_)
    {
        preferences.begin("wifi", true);
        *ssid_ = preferences.getString("ssid_string", "");
        *password_ = preferences.getString("password_string", "");
        preferences.end();
        return;
    }

    static void setProfile(int index, bool isOutside, int volume, int regularPeriod = 10, int cooldownPeriod = 0)
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

}

class WiFiHandler
{
public:
    WiFiHandler()
    {
        String *ssid, *password;
        Memory::getWiFiConfig(&ssid, &password);
        if (*ssid == "" || *password == "")
        {
            Serial.println("No wifi configuration.");
            // getSerialWifiConfig();
        }
        return;
    }

    WifiHandler(String ssid, String password)
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
        String *ssid, *password;
        Memory::getWiFiConfig(&ssid, &password);
        // initialize wifi
        if (*ssid != "")
        {
            WiFi.mode(WIFI_STA);
            WiFi.begin(*ssid, *password);
            Serial.println("Setting up Wifi.");

            // Wait for connection
            int timeout = 0;
            while (WiFi.status() != WL_CONNECTED && timeout < 30000)
            {
                Delay(500);
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
}

// https://www.weatherapi.com/docs/
// https://ipstack.com/
// https://wokwi.com/projects/371565043567756289
class APIWrapper
{
public:
    APIWrapper(char *url) : url(url)
    {
        data = "";
    }

    void getDataFromURL(String key)
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
                    Delay(2500);
                    return;
                }
            }
            Serial.println("Successful API call");
            http.end();
            data = doc[key].as<char *>() || -1;
        }
        else
        {
            Serial.println("WiFi not connected.");
            data = -1;
        }
        return;
    }

    char *getData()
    {
        return data;
    }

private:
    // api call
    char *url;
    DynamicJsonDocument doc(2048);
    char *data;
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
        Delay(time_ms);
        digitalWrite(pumpPin, false);
        Delay(10);
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

APIWrapper api('https://api.chucknorris.io/jokes/random');

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
    DynamicJsonDocument doc(1024);
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
    // Delay(10 * 60 * 1000); // 10 minutes
    Delay(10 * 1000);
}
