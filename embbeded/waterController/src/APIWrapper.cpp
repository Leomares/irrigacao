#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "WiFiHandler.h"
#include "APIWrapper.h"
#include "secrets.h"

// https://www.weatherapi.com/docs/
// https://ipstack.com/
// https://wokwi.com/projects/371565043567756289

// DynamicJsonDocument doc(256);
static StaticJsonDocument<512> doc;
static HTTPClient http;

APIWrapper::APIWrapper()
{
    interval = 15;
    url = api_url;

    precip_mm = 0.0;
    current_timestamp = 0;
}

void APIWrapper::getDataFromURL()
{
    if (!WiFiHandler::isConnected())
    {
        WiFiHandler::connectWifi();
    }

    if (WiFiHandler::isConnected())
    {
        Serial.println(F("Getting current data..."));
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
                delay(1000);
                return;
            }
        }
        Serial.println(F("Successful API call."));
        http.end();

        // data collection
        precip_mm = doc["current"]["precip_mm"] | 0;
        // strlcpy(last_updated, doc["current"]["last_updated"] | "YYYY-MM-DD HH:MM", 17);

        buffer[current_timestamp] = precip_mm;
        current_timestamp = (current_timestamp + 1) % (7 * 24 * 4);
    }
    else
    {
        Serial.println(F("WiFi not connected."));
    }
    return;
}

float APIWrapper::getData(int period)
{
    float accumulated = 0;
    int initial_index;
    if (current_timestamp * interval - period > 0)
    {
        initial_index = (current_timestamp * interval - period) / interval;
    }
    else
    {
        initial_index = 0;
    }
    for (int i = initial_index; i < current_timestamp; i++)
    {
        accumulated += buffer[(i) % (7 * 24 * 4)];
    }
    return accumulated;
}