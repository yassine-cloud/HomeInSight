#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>

class TimeService
{
public:
    void connectWiFi(const char *ssid, const char *password,
                     const char *ip, const char *gateway,
                     const char *subnet, const char *dns1, const char *dns2);
    void syncLocationAndTime();
    String getFormattedTime();
    String getDetectedLocation() const { return detectedLocation; }
    bool isLocationFetched() const { return locationFetched; } // Added getter

private:
    String detectedLocation = "Detecting...";
    bool locationFetched = false; // Added flag
    const char *ntpServer = "pool.ntp.org";
    const char *fallbackTZ = "CET-1";
};

#endif