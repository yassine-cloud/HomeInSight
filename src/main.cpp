#include <Arduino.h>
#include <esp_pm.h>
#include <esp_bt.h>
#include <atomic>
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

// Global Application State Data & Concurrency Guard
PowerData currentPowerData;
AnalyticsResult currentAnalytics;
std::atomic<bool> pendingReset{false}; 
SemaphoreHandle_t dataMutex = NULL;

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

    // Initialize state mutex
    dataMutex = xSemaphoreCreateMutex();

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
    webServer.begin(currentPowerData, currentAnalytics, timeService, pendingReset, dataMutex);
    Serial.println("[HTTP] Web server running on port 80");
}

void loop()
{
    static unsigned long lastSecTick = 0;
    static int lastResetDay = -1; 

    // Execute sensor read & analytics processing once every update cycle (2 seconds)
    if (millis() - lastSecTick >= SENSOR_READ_INTERVAL)
    {
        lastSecTick = millis();

        // 1. Fetch fresh hardware readings
        PowerData freshPower = powerSensor.readData();
        freshPower.pendingReset = pendingReset.load();

        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            AnalyticsResult freshAnalytics = analytics.update(freshPower.energy, freshPower.power, timeinfo);

            // Mutex-protected lock for main state update
            if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                currentPowerData = freshPower;
                currentAnalytics = freshAnalytics;
                xSemaphoreGive(dataMutex);
            }

            if (freshAnalytics.hourRolloverOccurred)
            {
                firebaseService.sendHourlyLog(freshAnalytics.hourlyLogKey,
                                              freshAnalytics.lastHourEnergy,
                                              freshPower.voltage,
                                              freshPower.current,
                                              freshPower.power);

                bool isScheduled4AM = (timeinfo.tm_hour == ENERGY_RESET_HOUR && lastResetDay != timeinfo.tm_mday);

                if (pendingReset.load() || isScheduled4AM)
                {
                    if (powerSensor.resetEnergy())
                    {
                        Serial.println("[SCHEDULE] PZEM hardware energy meter reset successful.");
                    }

                    if (isScheduled4AM)
                    {
                        lastResetDay = timeinfo.tm_mday;
                    }

                    // Delay 200ms to allow PZEM hardware registers to finalize clearing
                    delay(200);

                    // Re-read sensor metrics & reset analytics baseline
                    freshPower = powerSensor.readData();
                    analytics.resetBaseline(freshPower.energy);

                    pendingReset.store(false);
                    freshPower.pendingReset = false;

                    // Sync predicted energy before pushing top-of-hour telemetry
                    freshPower.predictedHourEnergy = freshAnalytics.predictedHourEnergy;

                    if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        currentPowerData = freshPower;
                        xSemaphoreGive(dataMutex);
                    }

                    firebaseService.sendLiveTelemetry(freshPower);
                }
            }
        }
    }

    // Telemetry clock boundary check
    if (firebaseService.isReadyForLiveUpdate(FIREBASE_LIVE_INTERVAL))
    {
        Serial.println("[FIREBASE] Sending live telemetry update...");
        PowerData telemetryCopy;
        if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            currentPowerData.predictedHourEnergy = currentAnalytics.predictedHourEnergy;
            telemetryCopy = currentPowerData;
            xSemaphoreGive(dataMutex);
        }
        else
        {
            telemetryCopy = currentPowerData;
            telemetryCopy.predictedHourEnergy = currentAnalytics.predictedHourEnergy;
        }
        firebaseService.sendLiveTelemetry(telemetryCopy);
    }

    delay(DELAY_LOOP_INTERVAL); 
}