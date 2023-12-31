// #include <EEPROM.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include "Memory.h"
#include "secrets.h"

static Preferences prefs;

Memory::Memory()
{
    return;
}

void Memory::setWiFiConfig(String ssid_, String password_)
{
    prefs.begin("wifi", false);
    prefs.putString("ssid_string", ssid_);
    prefs.putString("password_string", password_);
    prefs.end();
    return;
}

void Memory::getWiFiConfig(String *ssid_, String *password_)
{
    prefs.begin("wifi", true);
    *ssid_ = prefs.getString("ssid_string", "");
    *password_ = prefs.getString("password_string", "");
    prefs.end();
    return;
}

void Memory::setProfile(int index, Profile profile)
{
    char profile_name[15];
    sprintf(profile_name, "profile%d", index);
    prefs.begin(profile_name, false);
    prefs.putBool("valid", true);
    prefs.putBool("isOutside", profile.isOutside);
    prefs.putInt("volume", profile.volume);
    prefs.putInt("regularPeriod", profile.regularPeriod);
    prefs.putInt("cooldownPeriod", profile.cooldownPeriod);
    prefs.end();

    return;
}

Profile Memory::getProfile(int index)
{

    bool validProfile;
    Profile profile;

    char profile_name[15];
    sprintf(profile_name, "profile%d", index);
    prefs.begin(profile_name, true);
    validProfile = prefs.getBool("valid", false);
    if (!validProfile)
    {
        Serial.print(F("Index "));
        Serial.print(index);
        Serial.println(F(" is not a valid profile. Returning the default one"));
        prefs.end();
        sprintf(profile_name, "profile%d", -1);
        prefs.begin(profile_name, true);
    }
    profile.isOutside = prefs.getBool("isOutside", false);
    profile.volume = prefs.getInt("volume", 0);
    profile.regularPeriod = prefs.getInt("regularPeriod", 0);
    profile.cooldownPeriod = prefs.getInt("cooldownPeriod", 0);
    prefs.end();
    return profile;
}

int Memory::getLastProfile(int controllerIndex)
{
    int profileIndex;
    char controllerLastIndex[4];
    sprintf(controllerLastIndex, "%d", controllerIndex);
    prefs.begin("lastprofile", true);
    profileIndex = prefs.getInt(controllerLastIndex, -1);
    prefs.end();
    return profileIndex;
}

void Memory::setLastProfile(int controllerIndex, int profileIndex)
{
    char controllerLastIndex[4];
    sprintf(controllerLastIndex, "%d", controllerIndex);
    prefs.begin("lastprofile", false);
    prefs.putInt(controllerLastIndex, profileIndex);
    prefs.end();
    return;
}
#if DEFAULT_MEMORY_CONFIG
void Memory::setDefaultProfile()
{
    Profile currentProfile;
    currentProfile.volume = 20;
    currentProfile.regularPeriod = 0;
    currentProfile.cooldownPeriod = 10;
    currentProfile.isOutside = false;
    Memory::setProfile(-1, currentProfile);
    return;
}

void Memory::setDefaultWiFiConfig()
{
    Memory::setWiFiConfig(home_ssid, home_password);
    return;
}
void Memory::resetNVS()
{
    nvs_flash_erase(); // erase the NVS partition and...
    nvs_flash_init();  // initialize the NVS partition.
    return;
}
#endif