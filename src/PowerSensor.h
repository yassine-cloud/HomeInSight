#ifndef POWER_SENSOR_H
#define POWER_SENSOR_H

#include <Arduino.h>
#include <PZEM004Tv30.h>

struct PowerData
{
    float voltage = 0.0;
    float current = 0.0;
    float power = 0.0;
    float energy = 0.0;
    float frequency = 0.0;
    float pf = 0.0;
    float predictedHourEnergy = 0.0;
    bool pendingReset = false;        // Flag indicating energy reset is scheduled for top-of-hour
};

class PowerSensor
{
public:
    PowerSensor(uint8_t rxPin, uint8_t txPin);
    void begin();
    PowerData readData();
    bool resetEnergy(); // Send reset command to hardware meter

private:
    PZEM004Tv30 pzem;
    uint8_t _rxPin;
    uint8_t _txPin;
};

#endif