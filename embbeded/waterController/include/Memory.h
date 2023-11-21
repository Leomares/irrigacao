#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>

struct Profile
{
    bool isOutside;
    int volume;
    int regularPeriod;
    int cooldownPeriod;
};

class Memory
{
public:
    Memory();

    static void setWiFiConfig(String ssid_, String password_);

    static void getWiFiConfig(String *ssid_, String *password_);

    static void setProfile(int index, Profile profile);

    static Profile getProfile(int index);

    static void setDefaultProfile();

    static void setDefaultWiFiConfig();

    static void resetNVS();
};

#endif