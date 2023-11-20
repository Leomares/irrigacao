#ifndef API_WRAPPER_H
#define API_WRAPPER_H

/* APIWrapper creates an interface with the API from https://www.weatherapi.com/.
 * 1) Makes calls and store values of preciptation on a given location at a constant frequency. The default frequency is 15 minutes per call.
 * 2) The data is accumulated in different ranges (per hour, per day, per week) to accomodate different watering profiles.
 */
class APIWrapper
{
public:
    APIWrapper(int interval);
    void getDataFromURL();
    /* Returns the average preciptation for a given range (hourly at range 0, daily at range 1, weekly at range 2).
     * The precision depends on the frequency in which the api is called. Default is one request each 15 minutes.
     */
    float getData(int range);

private:
    const char *url;
    // interval between api requests in minutes.
    int interval;
    // timestamp of last api request with format "YYYY-MM-DD HH:MM".
    char *last_updated;
    float precip_mm;
    int current_timestamp;
    // accumulated preciptation data from three ranges: hourly, daily and weekly.
    float accumulated_precip_mm[3];
    float accumulated_timestamp[3];
};

#endif