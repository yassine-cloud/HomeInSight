// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- STATIC IP CONFIGURATION STRINGS ---
const String STR_LOCAL_IP = "192.168.1.150"; // The fixed IP address for your ESP32
const String STR_GATEWAY = "192.168.1.1";    // Your router IP address
const String STR_SUBNET = "255.255.255.0";   // Standard subnet mask
const String STR_PRIMARY_DNS = "8.8.8.8";    // Google DNS (Required for API & NTP domain resolution)
const String STR_SECONDARY_DNS = "8.8.4.4";  // Google DNS (Optional)

// Firebase Database Configuration
const unsigned long FIREBASE_LIVE_INTERVAL = 60000; // 60 seconds update interval

#endif
