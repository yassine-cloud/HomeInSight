#include "TimeService.h"

void TimeService::connectWiFi(const char *ssid, const char *password,
                              const char *ip, const char *gateway,
                              const char *subnet, const char *dns1, const char *dns2)
{
    IPAddress local_IP, gw, net, dnsPrimary, dnsSecondary;
    local_IP.fromString(ip);
    gw.fromString(gateway);
    net.fromString(subnet);
    dnsPrimary.fromString(dns1);
    dnsSecondary.fromString(dns2);

    if (!WiFi.config(local_IP, gw, net, dnsPrimary, dnsSecondary))
    {
        Serial.println("[WIFI] Static IP failed! Falling back to DHCP.");
    }

    WiFi.begin(ssid, password);
    Serial.print("[WIFI] Connecting to ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WIFI] Connected! IP: " + WiFi.localIP().toString());
}

void TimeService::syncLocationAndTime()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("[GEO] Contacting ipwho.is for location & timezone...");
        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;
        http.setTimeout(4000);
        http.begin(client, "https://ipwho.is/");

        int httpCode = http.GET();
        bool syncSuccess = false;

        Serial.printf("[GEO] HTTP Response Code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            Serial.println("[GEO] API Payload: " + payload);

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error && doc["success"] == true)
            {
                const char *city = doc["city"] | "Unknown";
                const char *country = doc["country"] | "Unknown";
                long utcOffset = doc["timezone"]["offset"] | 3600;

                detectedLocation = String(city) + ", " + String(country);
                configTime(utcOffset, 0, ntpServer);
                syncSuccess = true;
                Serial.printf("[GEO] Success! Location: %s (UTC Offset: %ld s)\n", detectedLocation.c_str(), utcOffset);
            }
            else
            {
                Serial.printf("[GEO] JSON Parse Error: %s\n", error.c_str());
            }
        }

        if (!syncSuccess)
        {
            Serial.println("[GEO] API lookup failed. Falling back to default timezone (CET-1).");
            detectedLocation = "Offline (Default TZ)";
            configTzTime(fallbackTZ, ntpServer);
        }
        http.end();
    }
    else
    {
        Serial.println("[GEO] Sync skipped: Wi-Fi disconnected!");
    }
}

String TimeService::getFormattedTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "Syncing...";
    }
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(timeStr);
}