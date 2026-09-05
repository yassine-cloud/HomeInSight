#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"
#include "config.h"

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// PZEM Serial pins
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

// Fallback Time Configuration (UTC+1 Tunisia baseline)
const char *ntpServer = "pool.ntp.org";
const char *fallbackTZ = "CET-1";

// Global Variables for Location & Server Data
String detectedLocation = "Detecting...";
String detectedTimezone = "CET-1";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Initialize PZEM sensor
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

// Global variables for live sensor data
float voltage = 0.0;
float current = 0.0;
float power = 0.0;
float energy = 0.0;
float frequency = 0.0;
float pf = 0.0;

// Hourly calculation tracking variables
bool firstFullHourStarted = false;
int lastTrackedHour = -1;
float hourStartEnergy = 0.0;
float lastHourEnergy = 0.0;
String lastHourTimeLabel = "Syncing NTP time...";
float predictedHourEnergy = 0.0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Power Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
  <style>
    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; max-width: 1200px; margin: 0 auto; }
    .card { background: white; border-radius: 15px; padding: 25px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: flex; align-items: center; text-align: left; }
    .icon { font-size: 40px; margin-right: 25px; min-width: 50px; text-align: center; }
    .content { display: flex; flex-direction: column; }
    .label { font-size: 16px; color: #7f8c8d; margin-bottom: 5px; font-weight: bold; }
    .sublabel { font-size: 12px; color: #bdc3c7; margin-top: 3px; }
    .value { font-size: 24px; color: #2c3e50; display: flex; align-items: baseline; }
    .unit { font-size: 16px; color: #95a5a6; margin-left: 5px; }
    .fa-bolt { color: #f1c40f; } .fa-exchange-alt { color: #3498db; } .fa-plug { color: #e74c3c; }
    .fa-chart-line { color: #2ecc71; } .fa-wave-square { color: #9b59b6; } .fa-percent { color: #e67e22; }
    .fa-history { color: #16a085; } .fa-calculator { color: #d35400; } .fa-map-marker-alt { color: #e74c3c; } .fa-clock { color: #8e44ad; }
    h1 { text-align: center; margin: 30px 0; color: #2c3e50; }
  </style>
  <script>
    function updateData() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          document.getElementById('voltage').innerHTML = data.voltage + '<span class="unit">V</span>';
          document.getElementById('current').innerHTML = data.current + '<span class="unit">A</span>';
          document.getElementById('power').innerHTML = data.power + '<span class="unit">W</span>';
          document.getElementById('energy').innerHTML = data.energy + '<span class="unit">kWh</span>';
          document.getElementById('frequency').innerHTML = data.frequency + '<span class="unit">Hz</span>';
          document.getElementById('pf').innerHTML = data.pf;
          document.getElementById('last_hour').innerHTML = data.last_hour + '<span class="unit">kWh</span>';
          document.getElementById('last_hour_label').innerText = data.last_hour_label;
          document.getElementById('predicted_hour').innerHTML = data.predicted_hour + '<span class="unit">kWh</span>';
          document.getElementById('location').innerText = data.location;
          document.getElementById('time').innerText = data.time;
        }
      };
      xhttp.open("GET", "/data", true);
      xhttp.send();
    }
    setInterval(updateData, 2000);
    window.onload = updateData;
  </script>
</head>
<body>
  <h1><i class="fas fa-plug"></i> ESP32 Power Monitor</h1>
  <div class="grid">
    <div class="card">
      <i class="fas fa-map-marker-alt icon"></i>
      <div class="content">
        <div class="label">LOCATION</div>
        <div class="value" id="location">%LOCATION%</div>
        <div class="sublabel">Auto-detected via Secure IP-API</div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-clock icon"></i>
      <div class="content">
        <div class="label">SYSTEM TIME</div>
        <div class="value" id="time">%TIME%</div>
        <div class="sublabel">Synchronized via NTP</div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-bolt icon"></i>
      <div class="content">
        <div class="label">VOLTAGE</div>
        <div class="value" id="voltage">%VOLTAGE%<span class="unit">V</span></div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-exchange-alt icon"></i>
      <div class="content">
        <div class="label">CURRENT</div>
        <div class="value" id="current">%CURRENT%<span class="unit">A</span></div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-plug icon"></i>
      <div class="content">
        <div class="label">POWER</div>
        <div class="value" id="power">%POWER%<span class="unit">W</span></div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-chart-line icon"></i>
      <div class="content">
        <div class="label">TOTAL ACCUMULATED</div>
        <div class="value" id="energy">%ENERGY%<span class="unit">kWh</span></div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-history icon"></i>
      <div class="content">
        <div class="label">LAST HOUR CONSUMPTION</div>
        <div class="value" id="last_hour">%LAST_HOUR%<span class="unit">kWh</span></div>
        <div class="sublabel" id="last_hour_label">%LAST_HOUR_LABEL%</div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-calculator icon"></i>
      <div class="content">
        <div class="label">PREDICTED THIS HOUR</div>
        <div class="value" id="predicted_hour">%PREDICTED_HOUR%<span class="unit">kWh</span></div>
        <div class="sublabel">Projected end-of-hour total</div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-wave-square icon"></i>
      <div class="content">
        <div class="label">FREQUENCY</div>
        <div class="value" id="frequency">%FREQUENCY%<span class="unit">Hz</span></div>
      </div>
    </div>
    <div class="card">
      <i class="fas fa-percent icon"></i>
      <div class="content">
        <div class="label">POWER FACTOR</div>
        <div class="value" id="pf">%PF%</div>
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";

void syncLocationAndTIme() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation for IoT clock sync to prevent expiration breakage

    HTTPClient http;
    http.setTimeout(4000); // 4-second safety timeout
    http.begin(client, "https://ipwho.is/"); // Secure HTTPS IP Geolocation API

    int httpCode = http.GET();
    bool syncSuccess = false;

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error && doc["success"] == true) {
        const char* city = doc["city"] | "Unknown";
        const char* country = doc["country"] | "Unknown";
        long utcOffset = doc["timezone"]["offset"] | 3600;

        detectedLocation = String(city) + ", " + String(country);
        
        // Configure NTP clock with detected offset
        configTime(utcOffset, 0, ntpServer);
        syncSuccess = true;
        Serial.printf("[GEO] Location identified: %s (UTC Offset: %ld s)\n", detectedLocation.c_str(), utcOffset);
      }
    }

    if (!syncSuccess) {
      Serial.println("[GEO] API lookup failed. Applying fallback baseline (CET-1).");
      detectedLocation = "Offline (Default TZ)";
      configTzTime(fallbackTZ, ntpServer);
    }
    http.end();
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Syncing...";
  }
  char timeStr[10];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(timeStr);
}

String processor(const String &var) {
  if (var == "VOLTAGE") return isnan(voltage) ? "Error" : String(voltage, 1);
  else if (var == "CURRENT") return isnan(current) ? "Error" : String(current, 3);
  else if (var == "POWER") return isnan(power) ? "Error" : String(power, 1);
  else if (var == "ENERGY") return isnan(energy) ? "Error" : String(energy, 3);
  else if (var == "FREQUENCY") return isnan(frequency) ? "Error" : String(frequency, 1);
  else if (var == "PF") return isnan(pf) ? "Error" : String(pf, 2);
  else if (var == "LAST_HOUR") return String(lastHourEnergy, 3);
  else if (var == "LAST_HOUR_LABEL") return lastHourTimeLabel;
  else if (var == "PREDICTED_HOUR") return String(predictedHourEnergy, 3);
  else if (var == "LOCATION") return detectedLocation;
  else if (var == "TIME") return getFormattedTime();
  return String();
}

// --- STATIC IP CONFIGURATION ---
// Global network objects
IPAddress local_IP;
IPAddress gateway;
IPAddress subnet;
IPAddress primaryDNS;
IPAddress secondaryDNS;

void setup() {
  Serial.begin(115200);

  // Initialize HardwareSerial2
  Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

    // --- CONVERT STRINGS TO IPADDRESS OBJECTS ---
  local_IP.fromString(STR_LOCAL_IP);
  gateway.fromString(STR_GATEWAY);
  subnet.fromString(STR_SUBNET);
  primaryDNS.fromString(STR_PRIMARY_DNS);
  secondaryDNS.fromString(STR_SECONDARY_DNS);

  // Configure static IP prior to connecting
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("[WIFI] Failed to configure Static IP! Falling back to DHCP.");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Execute Location and Time Sync over Secure HTTPS
  syncLocationAndTIme();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html, processor);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"voltage\":\"" + String(isnan(voltage) ? "Error" : String(voltage, 1)) + "\",";
    json += "\"current\":\"" + String(isnan(current) ? "Error" : String(current, 2)) + "\",";
    json += "\"power\":\"" + String(isnan(power) ? "Error" : String(power, 1)) + "\",";
    json += "\"energy\":\"" + String(isnan(energy) ? "Error" : String(energy, 3)) + "\",";
    json += "\"frequency\":\"" + String(isnan(frequency) ? "Error" : String(frequency, 1)) + "\",";
    json += "\"pf\":\"" + String(isnan(pf) ? "Error" : String(pf, 2)) + "\",";
    json += "\"last_hour\":\"" + String(lastHourEnergy, 3) + "\",";
    json += "\"last_hour_label\":\"" + lastHourTimeLabel + "\",";
    json += "\"predicted_hour\":\"" + String(predictedHourEnergy, 3) + "\",";
    json += "\"location\":\"" + detectedLocation + "\"";
    json += ",\"time\":\"" + getFormattedTime() + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  float v = pzem.voltage();
  float c = pzem.current();
  float p = pzem.power();
  float e = pzem.energy();
  float f = pzem.frequency();
  float pf_val = pzem.pf();

  // Update global values
  voltage = v;
  current = c;
  power = p;
  energy = e;
  frequency = f;
  pf = pf_val;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int currentHour = timeinfo.tm_hour;

    // Detect startup condition
    if (lastTrackedHour == -1) {
      lastTrackedHour = currentHour;
      lastHourTimeLabel = "Waiting for first full hour (e.g. " + String((currentHour + 1) % 24) + ":00)...";
    }

    // Detect top-of-the-hour boundary transition
    if (currentHour != lastTrackedHour) {
      if (firstFullHourStarted) {
        // Calculate consumption for the completed full hour
        lastHourEnergy = e - hourStartEnergy;
        if (lastHourEnergy < 0) lastHourEnergy = 0.0;

        char buf[35];
        snprintf(buf, sizeof(buf), "From %02d:00 To %02d:00", lastTrackedHour, currentHour);
        lastHourTimeLabel = String(buf);
      } else {
        // Ignore the partial startup hour and lock onto first full hour start
        firstFullHourStarted = true;
      }
      hourStartEnergy = e;
      lastTrackedHour = currentHour;
    }

    // Calculate prediction for the current active hour
    if (firstFullHourStarted) {
      float energyUsedSoFar = e - hourStartEnergy;
      if (energyUsedSoFar < 0) energyUsedSoFar = 0.0;

      float elapsedHours = (timeinfo.tm_min / 60.0) + (timeinfo.tm_sec / 3600.0);
      float remainingHours = 1.0 - elapsedHours;

      // Predicted Total = Energy consumed so far + (Current active power in kW * remaining time)
      predictedHourEnergy = energyUsedSoFar + ((isnan(power) ? 0.0 : power / 1000.0) * remainingHours);
    } else {
      // Prior to the first full hour, project purely off current power draw
      predictedHourEnergy = isnan(power) ? 0.0 : (power / 1000.0);
    }
  }

  delay(2000);
}