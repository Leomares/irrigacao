// #include <EEPROM.h>
#include <Preferences.h>

struct Profile
{
    bool isOutside;
    int volume;
    int regularPeriod;
    int cooldownPeriod;
};

Preferences prefs;

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

    static void setProfile(int index, Profile profile)
    {
        String profile = "profile" + String(index, DEC);
        prefs.begin(profile.c_str(), false);
        prefs.putBool("valid", true);
        prefs.putBool("isOutside", profile.isOutside);
        prefs.putInt("volume", profile.volume);
        prefs.putInt("regularPeriod", profile.regularPeriod);
        prefs.putInt("cooldownPeriod", profile.cooldownPeriod);
        prefs.end();

        return;
    }

    static Profile getProfile(int index)
    {

        bool validProfile;
        Profile profile;

        prefs.begin("profile" + to_string(index), true);
        validProfile = prefs.getBool("valid", false);
        if (!validProfile)
        {
            Serial.println((String) "Index" + index + "is not a valid profile.");
            return;
        }
        profile.isOutside = prefs.getBool("isOutside", false);
        profile.volume = prefs.getInt("volume", 0);
        profile.regularPeriod = prefs.getInt("regularPeriod", 0);
        profile.cooldownPeriod = prefs.getInt("cooldownPeriod", 0);
        prefs.end();
        return profile;
    }
};
