# Home In Sight

Home In Sight is an ESP32-based electricity monitoring system. It reads live electrical measurements from a PZEM-004T v4.0 energy meter, displays them through a local web dashboard, and stores telemetry in Firebase Realtime Database.

The firmware also calculates hourly energy information, predicts the current hour's energy usage, detects the device location through `ipwho.is`, and synchronizes local time through NTP.

## Features

- Measures voltage, current, active power, energy, frequency, and power factor.
- Reads the PZEM-004T over the ESP32 hardware UART.
- Connects to Wi-Fi using a configurable static IP address.
- Detects the approximate public-network location and timezone using `ipwho.is`.
- Synchronizes time using an NTP server.
- Serves a local dashboard from the ESP32 over HTTP.
- Exposes live dashboard data at the `/data` JSON endpoint.
- Sends live telemetry to Firebase every 60 seconds.
- Stores hourly consumption records in Firebase.
- Includes a Vite frontend that can be built and deployed to Firebase Hosting.

## System Overview

```text
PZEM-004T v4.0
        |
        | UART, 9600 baud
        v
ESP32 DevKit
_____________________
  |       |        |
  |       |        +--> Local HTTP dashboard and /data endpoint
  |       +-----------> Firebase Realtime Database
  +-------------------> Wi-Fi, geolocation API, and NTP time sync
```

## Hardware

### Required materials

- ESP32 DevKit development board
- PZEM-004T v4.0 AC energy meter
- Compatible current transformer (CT), if required by the selected PZEM installation
- AC load and suitable mains-rated wiring/connectors
- USB data cable for programming the ESP32
- 5 V USB power supply for the ESP32
- Jumper wires or a suitable enclosure and terminal blocks

### UART wiring

The firmware currently uses these pins:

| PZEM-004T v4.0 | ESP32 DevKit |
| --- | --- |
| TX | GPIO16 (ESP32 RX2) |
| RX | GPIO17 (ESP32 TX2) |
| GND | GND |
| VCC | Use the voltage specified by the PZEM module documentation |

The PZEM library uses `Serial2` at 9600 baud. The pin definitions are in `src/main.cpp` and can be changed if your board layout requires another UART mapping.

### Electrical safety

The PZEM-004T can be connected to hazardous mains voltage. Do not work on live wiring. Use appropriate fuses, insulation, enclosures, strain relief, and wire ratings. Installation and testing should be performed by a qualified person. Keep the low-voltage ESP32 and USB wiring isolated from mains conductors.

## Software Requirements

### Firmware

- Visual Studio Code
- PlatformIO IDE extension, or PlatformIO Core CLI
- USB driver for your ESP32 board, if required by its USB-to-serial chip
- A 2.4 GHz Wi-Fi network
- A Firebase project with Realtime Database enabled

### Optional web frontend deployment

- Node.js 18 or newer
- npm
- Firebase CLI
- A Firebase project with Hosting enabled

## Project Structure

```text
.
|-- include/
|   |-- config.h             Static network and application configuration
|   |-- secrets.example.h    Credentials template
|-- src/
|   |-- main.cpp             Firmware entry point and main loop
|   |-- PowerSensor.*        PZEM-004T measurement handling
|   |-- EnergyAnalytics.*    Hourly analytics and prediction
|   |-- FirebaseService.*    Firebase Realtime Database integration
|   |-- TimeService.*        Wi-Fi, geolocation, and NTP time handling
|   |-- WebServerManager.*   Local HTTP server and JSON endpoint
|   |-- WebDashboard.h       Embedded dashboard HTML
|-- web/
|   |-- index.html           Vite/Firebase Hosting dashboard
|   |-- package.json         Frontend scripts
|-- platformio.ini           PlatformIO environment and dependencies
|-- firebase.json            Firebase Hosting configuration
```

## Configuration

### 1. Create `secrets.h`

Copy the example file:

```bash
# Windows PowerShell
Copy-Item include/secrets.example.h include/secrets.h
```

Edit `include/secrets.h` and set your Wi-Fi and Firebase values:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "Your_Actual_SSID"
#define WIFI_PASSWORD "Your_Actual_Password"

#define FIREBASE_HOST "https://YOUR_DATABASE_NAME-default-rtdb.europe-west1.firebasedatabase.app/"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET_TOKEN"

#endif
```

Do not commit real credentials. `include/secrets.h` should remain local and is ignored by Git in this project.

### 2. Check the network configuration

Edit `include/config.h` if the defaults do not match your network:

| Setting | Default |
| --- | --- |
| ESP32 local IP | `192.168.1.150` |
| Gateway | `192.168.1.1` |
| Subnet | `255.255.255.0` |
| Primary DNS | `8.8.8.8` |
| Secondary DNS | `8.8.4.4` |
| Firebase live update interval | 60 seconds |

The ESP32 attempts to use the static address first and falls back to DHCP if static configuration fails. Make sure the selected address is outside your router's DHCP pool or reserved for this device.

### 3. Configure Firebase

Create a Firebase project and enable Realtime Database. The firmware uses these paths:

- `/live_telemetry`: latest measurement and predicted hourly energy.
- `/hourly_logs/<hour-key>`: hourly consumption, electrical values, and timestamp.

The current firmware uses Firebase legacy-token authentication. Use credentials appropriate for your Firebase setup and protect database rules before deploying outside a private network.

## Start the Firmware

Open the repository in VS Code with PlatformIO installed, connect the ESP32, and run:

```bash
pio run
pio run --target upload
pio device monitor --baud 115200
```

The equivalent PlatformIO toolbar actions are **Build**, **Upload**, and **Monitor**.

A complete clean rebuild can be run with:

```bash
pio run --target clean
pio run
```

At startup, the serial monitor reports Wi-Fi status, the assigned IP address, geolocation/time synchronization, Firebase initialization, and the HTTP server status.

## Use the Local Dashboard

After the ESP32 connects to Wi-Fi, open its IP address in a browser:

```text
http://192.168.1.150/
```

Replace the address with the IP printed by the serial monitor. The firmware also provides machine-readable data at:

```text
http://192.168.1.150/data
```

The dashboard displays voltage, current, power, accumulated energy, frequency, power factor, last-hour energy, predicted hourly energy, detected location, and local time.

## Build and Deploy the Web Frontend

The `web/` directory contains a Vite frontend that can be built for Firebase Hosting.

```bash
cd web
npm install
npm run dev
```

Create a production build:

```bash
npm run build
```

The build output is written to `web/dist`, which matches the `public` directory configured in `firebase.json`.

After authenticating with Firebase CLI and selecting the correct Firebase project, deploy the frontend from the repository root:

```bash
firebase login
firebase use <your-firebase-project-id>
cd web
npm run build
cd ..
firebase deploy --only hosting
```

The hosted frontend is separate from the dashboard embedded in the ESP32 firmware. The ESP32 dashboard is available locally even when the frontend has not been deployed.

## Troubleshooting

### The ESP32 does not connect to Wi-Fi

- Check `WIFI_SSID` and `WIFI_PASSWORD` in `include/secrets.h`.
- Confirm the network is 2.4 GHz and reachable from the ESP32.
- Check that the static IP, gateway, and subnet match the local network.
- Watch the serial monitor at 115200 baud.

### The sensor shows errors or zero values

- Confirm PZEM TX is connected to ESP32 GPIO16 and PZEM RX to GPIO17.
- Confirm the PZEM is powered correctly and shares a valid ground reference.
- Check the mains and CT installation according to the PZEM documentation.
- Keep high-voltage wiring disconnected while checking low-voltage UART wiring.

### Firebase does not receive data

- Verify `FIREBASE_HOST` and `FIREBASE_AUTH`.
- Confirm Realtime Database is enabled and its rules permit the requested writes.
- Confirm the ESP32 has working DNS and Internet access.
- Check the serial monitor for Wi-Fi and Firebase startup messages.

### Time or location is unavailable

The device calls `https://ipwho.is/` for approximate location and timezone information. If the request fails, the firmware falls back to the default timezone configured in `TimeService.cpp` and continues operating.

## Dependencies

PlatformIO installs these firmware libraries from `platformio.ini`:

- `mandulaj/PZEM-004T-v30`
- `me-no-dev/AsyncTCP`
- `ESPAsyncWebServer`
- `bblanchon/ArduinoJson`
- `Firebase Arduino Client Library for ESP8266 and ESP32`

The optional frontend uses Vite. Its browser dashboard also loads Font Awesome and Chart.js from public CDNs.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for the complete license text.

## Disclaimer

This project is provided for educational and monitoring purposes. Electrical measurements and mains installations can be dangerous. We are not responsible for damage, injury, inaccurate measurements, data loss, or other consequences resulting from use of this project.
