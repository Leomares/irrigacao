#include <HTTPClient.h>
#include <ArduinoJson.h>

// https://www.weatherapi.com/docs/
// https://ipstack.com/
// https://wokwi.com/projects/371565043567756289
DynamicJsonDocument doc(1024);
class APIWrapper
{
public:
    APIWrapper(char *url, int interval) : url(url)
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
            current_timestamp++;
            if (current_timestamp >= 24 * 60 / interval)
            {
                current_timestamp = 0;
                accumulated_precip = 0;
                accumulated_timestamp = 0;
            }
            if (precip_mm > 0)
            {
                accumulated_precip += precip_mm;
                accumulated_timestamp++;
            }
        }
        else
        {
            Serial.println("WiFi not connected.");
        }
        return;
    }

    void getData(&accumulated, &time)
    {
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
