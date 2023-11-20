#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

class WiFiHandler
{
public:
    WiFiHandler();

    WiFiHandler(char *ssid, char *password);

    static void getSerialWifiConfig();

    static void connectWifi();

    static bool isConnected();
};

#endif