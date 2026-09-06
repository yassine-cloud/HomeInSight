#include "WebServerManager.h"
#include "WebDashboard.h"

WebServerManager::WebServerManager(uint16_t port) : server(port) {}

void WebServerManager::begin(const PowerData &powerData, const AnalyticsResult &analytics, TimeService &timeService)
{
    // Handle browser favicon requests cleanly
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204); 
    });

    server.on("/", HTTP_GET, [&powerData, &analytics, &timeService](AsyncWebServerRequest *request)
    {
        auto processor = [&powerData, &analytics, &timeService](const String& var) -> String {
            if (var == "VOLTAGE") return isnan(powerData.voltage) ? "Error" : String(powerData.voltage, 1);
            if (var == "CURRENT") return isnan(powerData.current) ? "Error" : String(powerData.current, 3);
            if (var == "POWER") return isnan(powerData.power) ? "Error" : String(powerData.power, 1);
            if (var == "ENERGY") return isnan(powerData.energy) ? "Error" : String(powerData.energy, 3);
            if (var == "FREQUENCY") return isnan(powerData.frequency) ? "Error" : String(powerData.frequency, 1);
            if (var == "PF") return isnan(powerData.pf) ? "Error" : String(powerData.pf, 2);
            if (var == "LAST_HOUR") return String(analytics.lastHourEnergy, 3);
            if (var == "LAST_HOUR_LABEL") return analytics.lastHourTimeLabel;
            if (var == "PREDICTED_HOUR") return String(analytics.predictedHourEnergy, 3);
            if (var == "LOCATION") return timeService.getDetectedLocation();
            if (var == "TIME") return timeService.getFormattedTime();
            return String();
        };
        request->send(200, "text/html", index_html, processor); 
    });

    server.on("/data", HTTP_GET, [&powerData, &analytics, &timeService](AsyncWebServerRequest *request)
    {
        String json = "{";
        json += "\"voltage\":\"" + String(isnan(powerData.voltage) ? "Error" : String(powerData.voltage, 1)) + "\",";
        json += "\"current\":\"" + String(isnan(powerData.current) ? "Error" : String(powerData.current, 3)) + "\",";
        json += "\"power\":\"" + String(isnan(powerData.power) ? "Error" : String(powerData.power, 1)) + "\",";
        json += "\"energy\":\"" + String(isnan(powerData.energy) ? "Error" : String(powerData.energy, 3)) + "\",";
        json += "\"frequency\":\"" + String(isnan(powerData.frequency) ? "Error" : String(powerData.frequency, 1)) + "\",";
        json += "\"pf\":\"" + String(isnan(powerData.pf) ? "Error" : String(powerData.pf, 2)) + "\",";
        json += "\"last_hour\":\"" + String(analytics.lastHourEnergy, 3) + "\",";
        json += "\"last_hour_label\":\"" + analytics.lastHourTimeLabel + "\",";
        json += "\"predicted_hour\":\"" + String(analytics.predictedHourEnergy, 3) + "\",";
        json += "\"location\":\"" + timeService.getDetectedLocation() + "\",";
        json += "\"time\":\"" + timeService.getFormattedTime() + "\"";
        json += "}";
        request->send(200, "application/json", json); 
    });

    server.begin();
}