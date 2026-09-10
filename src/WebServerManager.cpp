#include "WebServerManager.h"
#include "WebDashboard.h"

WebServerManager::WebServerManager(uint16_t port) : server(port) {}

void WebServerManager::begin(const PowerData &powerData, const AnalyticsResult &analytics, TimeService &timeService, std::atomic<bool> &pendingResetFlag, SemaphoreHandle_t dataMutex)
{
    pendingResetPtr = &pendingResetFlag;
    this->dataMutex = dataMutex;

    // Handle browser favicon requests cleanly
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(204); });

    // Endpoint to toggle next-hour reset flag safely across tasks
    server.on("/api/toggle-reset", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        if (pendingResetPtr)
        {
            bool newState = !pendingResetPtr->load();
            pendingResetPtr->store(newState);
            request->send(200, "application/json", "{\"success\":true,\"pendingReset\":" + String(newState ? "true" : "false") + "}");
        }
        else
        {
            request->send(500, "application/json", "{\"error\":\"Uninitialized\"}");
        } });

    server.on("/", HTTP_GET, [this, &powerData, &analytics, &timeService](AsyncWebServerRequest *request)
              {
        auto processor = [this, &powerData, &analytics, &timeService](const String& var) -> String {
            PowerData localPower;
            AnalyticsResult localAnalytics;

            if (this->dataMutex && xSemaphoreTake(this->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                localPower = powerData;
                localAnalytics = analytics;
                xSemaphoreGive(this->dataMutex);
            } else {
                localPower = powerData;
                localAnalytics = analytics;
            }

            if (var == "VOLTAGE") return isnan(localPower.voltage) ? "Error" : String(localPower.voltage, 1);
            if (var == "CURRENT") return isnan(localPower.current) ? "Error" : String(localPower.current, 3);
            if (var == "POWER") return isnan(localPower.power) ? "Error" : String(localPower.power, 1);
            if (var == "ENERGY") return isnan(localPower.energy) ? "Error" : String(localPower.energy, 3);
            if (var == "FREQUENCY") return isnan(localPower.frequency) ? "Error" : String(localPower.frequency, 1);
            if (var == "PF") return isnan(localPower.pf) ? "Error" : String(localPower.pf, 2);
            if (var == "LAST_HOUR") return String(localAnalytics.lastHourEnergy, 3);
            if (var == "LAST_HOUR_LABEL") return localAnalytics.lastHourTimeLabel;
            if (var == "PREDICTED_HOUR") return String(localAnalytics.predictedHourEnergy, 3);
            if (var == "LOCATION") return timeService.getDetectedLocation();
            if (var == "TIME") return timeService.getFormattedTime();
            if (var == "RESET_STATUS") return (pendingResetPtr && pendingResetPtr->load()) ? "Cancel Reset" : "Schedule Reset";
            return String();
        };
        request->send(200, "text/html", index_html, processor); });

    // Optimized endpoint using stack buffer to prevent heap fragmentation
    server.on("/data", HTTP_GET, [this, &powerData, &analytics, &timeService](AsyncWebServerRequest *request)
              {
        PowerData localPower;
        AnalyticsResult localAnalytics;

        if (this->dataMutex && xSemaphoreTake(this->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            localPower = powerData;
            localAnalytics = analytics;
            xSemaphoreGive(this->dataMutex);
        } else {
            localPower = powerData;
            localAnalytics = analytics;
        }

        char vBuf[10], cBuf[10], pBuf[10], eBuf[10], fBuf[10], pfBuf[10];
        snprintf(vBuf, sizeof(vBuf), isnan(localPower.voltage) ? "Error" : "%.1f", localPower.voltage);
        snprintf(cBuf, sizeof(cBuf), isnan(localPower.current) ? "Error" : "%.3f", localPower.current);
        snprintf(pBuf, sizeof(pBuf), isnan(localPower.power) ? "Error" : "%.1f", localPower.power);
        snprintf(eBuf, sizeof(eBuf), isnan(localPower.energy) ? "Error" : "%.3f", localPower.energy);
        snprintf(fBuf, sizeof(fBuf), isnan(localPower.frequency) ? "Error" : "%.1f", localPower.frequency);
        snprintf(pfBuf, sizeof(pfBuf), isnan(localPower.pf) ? "Error" : "%.2f", localPower.pf);

        char jsonBuf[512];
        snprintf(jsonBuf, sizeof(jsonBuf),
            "{"
            "\"voltage\":\"%s\","
            "\"current\":\"%s\","
            "\"power\":\"%s\","
            "\"energy\":\"%s\","
            "\"frequency\":\"%s\","
            "\"pf\":\"%s\","
            "\"last_hour\":\"%.3f\","
            "\"last_hour_label\":\"%s\","
            "\"pending_reset\":%s,"
            "\"predicted_hour\":\"%.3f\","
            "\"location\":\"%s\","
            "\"time\":\"%s\""
            "}",
            vBuf, cBuf, pBuf, eBuf, fBuf, pfBuf,
            localAnalytics.lastHourEnergy,
            localAnalytics.lastHourTimeLabel.c_str(),
            (pendingResetPtr && pendingResetPtr->load()) ? "true" : "false",
            localAnalytics.predictedHourEnergy,
            timeService.getDetectedLocation().c_str(),
            timeService.getFormattedTime().c_str()
        );

        request->send(200, "application/json", jsonBuf); });

    server.begin();
}