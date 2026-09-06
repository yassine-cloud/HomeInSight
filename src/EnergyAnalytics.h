#ifndef ENERGY_ANALYTICS_H
#define ENERGY_ANALYTICS_H

#include <Arduino.h>
#include <time.h>

struct AnalyticsResult
{
    bool hourRolloverOccurred = false;
    float lastHourEnergy = 0.0;
    String lastHourTimeLabel = "Syncing NTP time...";
    String hourlyLogKey = "";
    float predictedHourEnergy = 0.0;
};

class EnergyAnalytics
{
public:
    AnalyticsResult update(float currentEnergy, float currentPower, const struct tm &timeinfo);

private:
    bool firstFullHourStarted = false;
    int lastTrackedHour = -1;
    float hourStartEnergy = 0.0;
    float lastHourEnergy = 0.0;
    String lastHourTimeLabel = "Waiting for first full hour...";
};

#endif