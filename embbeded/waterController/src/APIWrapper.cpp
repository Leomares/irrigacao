#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "WiFiHandler.h"
#include "APIWrapper.h"
#include "secrets.h"

// https://www.weatherapi.com/docs/
// https://ipstack.com/
// https://wokwi.com/projects/371565043567756289

DynamicJsonDocument doc(1024);
HTTPClient http;

const int timer_ranges[] = {60, 24 * 60, 7 * 24 * 60};

APIWrapper::APIWrapper(int interval = 15)
{
    this->interval = interval;
    url = api_url;

    precip_mm = 0.0;
    strlcpy(last_updated, (char *)"YYYY-MM-DD HH:MM", 17);

    current_timestamp = 0;
    for (int i = 0; i < 3; i++)
    {
        accumulated_precip_mm[i] = 0;
        accumulated_timestamp[i] = 0;
    }
}

void APIWrapper::getDataFromURL()
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
        strlcpy(last_updated, doc["current"]["last_updated"] | "YYYY-MM-DD HH:MM", 17);
        precip_mm = doc["current"]["precip_mm"] | 0;

        for (int i = 2; i >= 0; i--)
        {
            if (accumulated_timestamp[i] > timer_ranges[i])
            {
                accumulated_precip_mm[i] = 0;
                accumulated_timestamp[i] = 0;
            }
            accumulated_precip_mm[i] += precip_mm;
            accumulated_timestamp[i] += interval;
        }
    }
    else
    {
        Serial.println("WiFi not connected.");
    }
    return;
}

float APIWrapper::getData(int range)
{
    float accumulated = accumulated_precip_mm[range] * accumulated_timestamp[range] / timer_ranges[range];
    return accumulated;
}
