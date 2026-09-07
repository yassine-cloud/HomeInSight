#include "EnergyAnalytics.h"

AnalyticsResult EnergyAnalytics::update(float currentEnergy, float currentPower, const struct tm &timeinfo)
{
    AnalyticsResult result;
    int currentHour = timeinfo.tm_hour;

    if (lastTrackedHour == -1)
    {
        lastTrackedHour = currentHour;
    }

    if (currentHour != lastTrackedHour)
    {
        if (firstFullHourStarted)
        {
            lastHourEnergy = currentEnergy - hourStartEnergy;
            if (lastHourEnergy < 0)
                lastHourEnergy = 0.0;

            char labelBuf[35];
            snprintf(labelBuf, sizeof(labelBuf), "From %02d:00 To %02d:00", lastTrackedHour, currentHour);
            lastHourTimeLabel = String(labelBuf);

            // Shift timestamp back 1 hour to get the correct date for the completed hour
            struct tm loggedTime = timeinfo;
            loggedTime.tm_isdst = -1; // Force auto-detection of DST rules
            time_t loggedEpoch = mktime(&loggedTime) - 3600;
            localtime_r(&loggedEpoch, &loggedTime);

            char keyBuf[30];
            snprintf(keyBuf, sizeof(keyBuf), "%04d-%02d-%02d_%02d:00",
                     loggedTime.tm_year + 1900,
                     loggedTime.tm_mon + 1,
                     loggedTime.tm_mday,
                     loggedTime.tm_hour); // Uses loggedTime.tm_hour to guarantee sync with the date

            result.hourRolloverOccurred = true;
            result.lastHourEnergy = lastHourEnergy;
            result.hourlyLogKey = String(keyBuf);
        }
        else
        {
            firstFullHourStarted = true;
        }
        hourStartEnergy = currentEnergy;
        lastTrackedHour = currentHour;
    }

    result.lastHourEnergy = lastHourEnergy;
    result.lastHourTimeLabel = lastHourTimeLabel;

    if (firstFullHourStarted)
    {
        float energyUsedSoFar = currentEnergy - hourStartEnergy;
        if (energyUsedSoFar < 0)
            energyUsedSoFar = 0.0;
        float elapsedHours = (timeinfo.tm_min / 60.0) + (timeinfo.tm_sec / 3600.0);
        float remainingHours = 1.0 - elapsedHours;
        result.predictedHourEnergy = energyUsedSoFar + ((isnan(currentPower) ? 0.0 : currentPower / 1000.0) * remainingHours);
    }
    else
    {
        result.predictedHourEnergy = isnan(currentPower) ? 0.0 : (currentPower / 1000.0);
    }

    return result;
}