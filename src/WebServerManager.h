#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <atomic>
#include "PowerSensor.h"
#include "EnergyAnalytics.h"
#include "TimeService.h"

class WebServerManager
{
public:
    WebServerManager(uint16_t port);
    void begin(const PowerData &powerData, const AnalyticsResult &analytics, TimeService &timeService, std::atomic<bool> &pendingResetFlag, SemaphoreHandle_t dataMutex);

private:
    AsyncWebServer server;
    std::atomic<bool> *pendingResetPtr = nullptr;
    SemaphoreHandle_t dataMutex = nullptr;
};

#endif