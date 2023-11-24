#define WIFI_USE 1
#if WIFI_USE

#include <Arduino.h>
#include <WiFi.h>
#include "WiFiHandler.h"
#include "Memory.h"

const int timeout_threshold = 60000;

WiFiHandler::WiFiHandler()
{
    String ssid, password;
    Memory::getWiFiConfig(&ssid, &password);
    if (ssid == "" || password == "")
    {
        Serial.println("No wifi configuration.");
        // getSerialWifiConfig();
    }
    return;
}

WiFiHandler::WiFiHandler(char *ssid, char *password)
{
    Memory::setWiFiConfig(ssid, password);
    return;
}

void WiFiHandler::getSerialWifiConfig()
{
    String ssid;
    String password;
    String response = "";

    Serial.print("Configure Wifi?");
    response = Serial.readString();
    if (response == "y")
    {
        // pass name and password to wifi namespace
        Serial.println("SSID to connect: ");
        ssid = Serial.readString();
        Serial.println("password: ");
        password = Serial.readString();
        Memory::setWiFiConfig(ssid, password);
    }
    return;
}

void WiFiHandler::connectWifi()
{
    // read wifi config
    String ssid, password;
    Memory::getWiFiConfig(&ssid, &password);
    // initialize wifi
    if (ssid != "")
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.println("Setting up Wifi.");

        // Wait for connection
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < timeout_threshold)
        {
            delay(500);
            timeout += 500;
            Serial.print(".");
        }
        if (timeout < timeout_threshold)
        {
            Serial.print("WiFi connected with IP: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.println("Wifi connection timeout.");
        }
    }
    else
    {
        Serial.println("No Wifi config found.");
    }
    return;
}

bool WiFiHandler::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
#endif