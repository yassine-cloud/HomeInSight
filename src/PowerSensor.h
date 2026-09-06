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
};

class PowerSensor
{
public:
    PowerSensor(uint8_t rxPin, uint8_t txPin);
    void begin();
    PowerData readData();

private:
    PZEM004Tv30 pzem;
    uint8_t _rxPin;
    uint8_t _txPin;
};

#endif