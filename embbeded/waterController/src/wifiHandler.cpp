#include <WiFi.h>

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
