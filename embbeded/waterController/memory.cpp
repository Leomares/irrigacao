// #include <EEPROM.h>
#include <Preferences.h> //storage in flash
using namespace std;

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

    static void setProfile(int index, bool isOutside, int volume, int regularPeriod = 10, int cooldownPeriod = 0)
    {
        String profile = "profile" + String(index,DEC);
        prefs.begin(profile.c_str(), false);
        prefs.putBool("valid", true);
        prefs.putBool("isOutside", isOutside);
        prefs.putInt("volume", volume);
        prefs.putInt("regularPeriod", regularPeriod);
        prefs.putInt("cooldownPeriod", cooldownPeriod);
        prefs.end();

        return;
    }

};
