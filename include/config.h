// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- STATIC IP CONFIGURATION STRINGS ---
#define STR_LOCAL_IP "192.168.1.150" // The fixed IP address for your ESP32
#define STR_GATEWAY "192.168.1.1"    // Your router IP address
#define STR_SUBNET "255.255.255.0"   // Standard subnet mask
#define STR_PRIMARY_DNS "8.8.8.8"    // Google DNS (Required for API & NTP domain resolution)
#define STR_SECONDARY_DNS "8.8.4.4"  // Google DNS (Optional)

// Firebase Database Configuration
const uint32_t  FIREBASE_LIVE_INTERVAL = 60; // 60 seconds update interval
const uint32_t  SENSOR_READ_INTERVAL = 2000; // 2 seconds update interval
const uint32_t  DELAY_LOOP_INTERVAL = 500; // 500ms delay loop interval

// --- ENERGY COUNTER RESET CONFIGURATION ---
const uint8_t ENERGY_RESET_HOUR   = 4; // Reset hour (04:00 AM)

// --- GEOLOCATION RETRY CONFIGURATION ---
const uint32_t GEO_RETRY_INTERVAL = 10000;  // Retry every 10 seconds if geolocation fetch fails
#endif
