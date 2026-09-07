#ifndef FIREBASE_SERVICE_H
#define FIREBASE_SERVICE_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "PowerSensor.h"

class FirebaseService
{
public:
    void begin(const char *host, const char *authKey);
    void sendLiveTelemetry(const PowerData &data);
    void sendHourlyLog(const String &hourKey, float kwh, float v, float c, float p);
    bool isReadyForLiveUpdate(uint32_t intervalSeconds);

private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    time_t lastLoggedSegment = 0; // Tracks epoch time bucket
};

#endif