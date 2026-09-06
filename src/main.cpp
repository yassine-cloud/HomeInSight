#include <Arduino.h>
#include "secrets.h"
#include "config.h"
#include "PowerSensor.h"
#include "TimeService.h"
#include "EnergyAnalytics.h"
#include "FirebaseService.h"
#include "WebServerManager.h"

// Define Hardware Pins
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

// Global Service Instances
PowerSensor powerSensor(PZEM_RX_PIN, PZEM_TX_PIN);
TimeService timeService;
EnergyAnalytics analytics;
FirebaseService firebaseService;
WebServerManager webServer(80);

// Global Application State Data
PowerData currentPowerData;
AnalyticsResult currentAnalytics;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=================================");
    Serial.println("  ESP32 POWER MONITOR STARTING   ");
    Serial.println("=================================");

    // Initialize PZEM Sensor UART
    powerSensor.begin();

    // Connect Network
    timeService.connectWiFi(WIFI_SSID, WIFI_PASSWORD, STR_LOCAL_IP, STR_GATEWAY, STR_SUBNET, STR_PRIMARY_DNS, STR_SECONDARY_DNS);

    // Sync Geolocation and Time
    timeService.syncLocationAndTime();

    // Initialize Firebase RTDB
    firebaseService.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Serial.println("[FIREBASE] Initialized.");

    // Start Asynchronous Web Server
    webServer.begin(currentPowerData, currentAnalytics, timeService);
    Serial.println("[HTTP] Web server running on port 80");
}

void loop()
{
    // Read Hardware Metrics
    currentPowerData = powerSensor.readData();

    // Process Analytics & Hour Rollovers
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        currentAnalytics = analytics.update(currentPowerData.energy, currentPowerData.power, timeinfo);

        if (currentAnalytics.hourRolloverOccurred)
        {
            Serial.println("[ANALYTICS] Top of the hour! Logging hourly data to Firebase...");
            firebaseService.sendHourlyLog(currentAnalytics.hourlyLogKey,
                                          currentAnalytics.lastHourEnergy,
                                          currentPowerData.voltage,
                                          currentPowerData.current,
                                          currentPowerData.power);
        }
    }

    // Handle Periodic Live Telemetry Updates (Every 60 Seconds)
    if (firebaseService.isReadyForLiveUpdate(60000))
    {
        Serial.println("[FIREBASE] Sending live telemetry update...");
        currentPowerData.predictedHourEnergy = currentAnalytics.predictedHourEnergy;
        firebaseService.sendLiveTelemetry(currentPowerData);
    }

    delay(2000);
}