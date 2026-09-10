#include <Arduino.h>
#include <esp_pm.h>
#include <esp_bt.h>
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
    // 1. Drop CPU speed to 80 MHz
    setCpuFrequencyMhz(80);

    // 2. Turn off unused Bluetooth radio
    btStop();

    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=================================");
    Serial.println("  ESP32 POWER MONITOR STARTING   ");
    Serial.println("=================================");

    // Initialize PZEM Sensor UART
    powerSensor.begin();

    // Connect Network
    timeService.connectWiFi(WIFI_SSID, WIFI_PASSWORD, STR_LOCAL_IP, STR_GATEWAY, STR_SUBNET, STR_PRIMARY_DNS, STR_SECONDARY_DNS);

    // 3. Enable Wi-Fi Modem Sleep
    WiFi.setSleep(WIFI_PS_MIN_MODEM);

    // 4. Configure automatic light sleep during delay() execution
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 10,
        .light_sleep_enable = true};
    esp_pm_configure(&pm_config);

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
    static unsigned long lastSecTick = 0;
    static int lastResetDay = -1; // Tracks calendar day to execute reset strictly once daily

    // Execute sensor read & analytics processing once every update cycle (2 seconds)
    if (millis() - lastSecTick >= SENSOR_READ_INTERVAL)
    {
        lastSecTick = millis();

        // 1. Fetch fresh hardware readings
        currentPowerData = powerSensor.readData();

        // 2. Process analytics with newly updated sensor metrics
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            currentAnalytics = analytics.update(currentPowerData.energy, currentPowerData.power, timeinfo);

            if (currentAnalytics.hourRolloverOccurred)
            {
                // Serial.println("[ANALYTICS] Top of the hour! Logging hourly data to Firebase...");
                firebaseService.sendHourlyLog(currentAnalytics.hourlyLogKey,
                                              currentAnalytics.lastHourEnergy,
                                              currentPowerData.voltage,
                                              currentPowerData.current,
                                              currentPowerData.power);
            }

            // 3. Automated Daily Energy Counter Reset at 04:00 AM (configured in config.h)
            if (timeinfo.tm_hour == DAILY_RESET_HOUR && timeinfo.tm_min == DAILY_RESET_MINUTE)
            {
                if (lastResetDay != timeinfo.tm_mday)
                {
                    lastResetDay = timeinfo.tm_mday;
                    // Serial.println("[SCHEDULE] Executing 04:00 AM daily energy counter reset...");

                    if (powerSensor.resetEnergy())
                    {
                        // Serial.println("[SCHEDULE] PZEM hardware energy meter reset successful.");
                    }
                    else
                    {
                        // Serial.println("[SCHEDULE] PZEM hardware reset failed or not responding.");
                    }

                    // Re-read sensor metrics so local variables reflect zero energy
                    currentPowerData = powerSensor.readData();

                    // Adjust analytics baseline to 0.0 kWh for current hour calculation
                    analytics.resetBaseline(currentPowerData.energy);
                }
            }
        }
    }

    // 4. Telemetry clock boundary check (runs every DELAY_LOOP_INTERVAL ms to catch top-of-minute instantly)
    if (firebaseService.isReadyForLiveUpdate(FIREBASE_LIVE_INTERVAL))
    {
        Serial.println("[FIREBASE] Sending live telemetry update...");
        currentPowerData.predictedHourEnergy = currentAnalytics.predictedHourEnergy;
        firebaseService.sendLiveTelemetry(currentPowerData);
    }

    delay(DELAY_LOOP_INTERVAL); // FreeRTOS yield window
}