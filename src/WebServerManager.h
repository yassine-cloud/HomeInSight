#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "PowerSensor.h"
#include "EnergyAnalytics.h"
#include "TimeService.h"

class WebServerManager
{
public:
    WebServerManager(uint16_t port);
    void begin(const PowerData &powerData, const AnalyticsResult &analytics, TimeService &timeService, bool &pendingResetFlag);

private:
    AsyncWebServer server;
    bool *pendingResetPtr = nullptr;
};

#endif