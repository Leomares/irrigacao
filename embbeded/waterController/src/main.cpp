// todo
// separate in files
// implement memory access as static method of a class
// put profile in struct
// initialize timers and pass pointer to control func
// update control
// - remove overflow handling
// - estimate quantity of water needed after rain based on volume (precip_mm + time elapsed + area of the plant)
// implement timer to update location data and rain data
// implement bluetooth handler

#include "Memory.h" //storage in flash
#include "WiFiHandler.h"
#include "APIWrapper.h" //storage in flash
#include "Controller.h" //storage in flash

APIWrapper api('https://api.chucknorris.io/jokes/random', 15);

int n_controllers = 0;
int sensorPins[4] = {-1, -1, -1, -1};
int pumpPins[4] = {-1, -1, -1, -1};
Plant *profiles[4] = {};

void setup()
{
    // put your setup code here, to run once:

    Serial.begin(115200);
    Serial.print('Setting up profiles');
    for (int i = 0; i < 4; i++)
    {
        // update to read the first variable of a profile on flash?
        // create a namespace to load the last profile used on these pins
        if (sensorPins[i] < 0 || pumpPins[i] < 0)
        {
            break;
        }
        n_controllers++;
        profiles[i] = new Plant(sensorPins[i], pumpPins[i]);
        profiles[i]->setProfile(i);
    }

    // set bluetooth
}

void loop()
{
    // put your main code here, to run repeatedly:
    api.getDataFromURL("value");
    Serial.println(api.getData());
    if (WiFi.status() != WL_CONNECTED)
    {
        api.connectWifi();
    }
    api.getDataFromURL();
    char *rainData = api.getRainData("value");
    for (int i = 0; i < n_controllers; i++)
    {
        profiles[i].control(rainData);
    }
    // delay(10 * 60 * 1000); // 10 minutes
    delay(10 * 1000);
}
