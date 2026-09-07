#include "FirebaseService.h"
#include <math.h>

// Helper function to round float values to exact decimal places as double
static double roundTo(float val, int decimals)
{
    if (isnan(val))
        return 0.0;
    double scale = pow(10.0, decimals);
    return round((double)val * scale) / scale;
}

void FirebaseService::begin(const char *host, const char *authKey)
{
    config.database_url = host;
    config.signer.tokens.legacy_token = authKey;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

bool FirebaseService::isReadyForLiveUpdate(uint32_t intervalSeconds)
{
    if (intervalSeconds == 0)
        return false;

    time_t now;
    time(&now);

    // Guard: Do not trigger if NTP time has not synced yet
    if (now < 1600000000)
        return false;

    time_t currentSegment = now / intervalSeconds;

    if (currentSegment != lastLoggedSegment)
    {
        lastLoggedSegment = currentSegment;
        return true;
    }
    return false;
}

void FirebaseService::sendLiveTelemetry(const PowerData &data)
{
    if (!Firebase.ready())
        return;

    FirebaseJson json;
    time_t now;
    time(&now);

    // Rounding values prevents binary float precision noise in Firebase JSON
    json.set("voltage", roundTo(data.voltage, 1));
    json.set("current", roundTo(data.current, 3));
    json.set("power", roundTo(data.power, 1));
    json.set("energy", roundTo(data.energy, 3));
    json.set("frequency", roundTo(data.frequency, 1));
    json.set("power_factor", roundTo(data.pf, 2));
    json.set("predicted_hour_energy", roundTo(data.predictedHourEnergy, 3));
    json.set("timestamp", (unsigned long)now);

    Firebase.RTDB.setJSON(&fbdo, "/live_telemetry", &json);
}

void FirebaseService::sendHourlyLog(const String &hourKey, float kwh, float v, float c, float p)
{
    if (!Firebase.ready())
        return;

    FirebaseJson json;
    time_t now;
    time(&now);

    json.set("kwh_consumed", roundTo(kwh, 3));
    json.set("voltage", roundTo(v, 1));
    json.set("current", roundTo(c, 3));
    json.set("power", roundTo(p, 1));
    json.set("timestamp", (unsigned long)now);

    String path = "/hourly_logs/";
    path += hourKey;
    Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);
}