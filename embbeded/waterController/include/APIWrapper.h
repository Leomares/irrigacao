#ifndef API_WRAPPER_H
#define API_WRAPPER_H

/* APIWrapper creates an interface with the API from https://www.weatherapi.com/.
 * 1) Makes calls and store values of preciptation on a given location at a constant frequency. The default frequency is 15 minutes per call.
 * 2) The data is accumulated in a ring buffer to accomodate different watering profiles.
 */
class APIWrapper
{
public:
    APIWrapper();
    void getDataFromURL();
    /* Returns the average preciptation for a given period in minutes.
     * The precision depends on the frequency in which the api is called. Default is one request each 15 minutes.
     */
    float getData(int period);

private:
    const char *url;
    // interval between api requests in minutes.
    int interval;
    // timestamp of last api request with format "YYYY-MM-DD HH:MM".
    char *last_updated;
    float precip_mm;
    int current_timestamp;
    // accumulated preciptation data.
    float buffer[7 * 24 * 60 / 15];
};

#endif