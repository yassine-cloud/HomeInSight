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

            char keyBuf[30];
            snprintf(keyBuf, sizeof(keyBuf), "%04d-%02d-%02d_%02d:00",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, lastTrackedHour);

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