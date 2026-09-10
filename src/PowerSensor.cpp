#include "PowerSensor.h"

PowerSensor::PowerSensor(uint8_t rxPin, uint8_t txPin)
    : pzem(Serial2, rxPin, txPin), _rxPin(rxPin), _txPin(txPin) {}

void PowerSensor::begin()
{
    Serial2.begin(9600, SERIAL_8N1, _rxPin, _txPin);
}

PowerData PowerSensor::readData()
{
    PowerData data;
    data.voltage = pzem.voltage();
    data.current = pzem.current();
    data.power = pzem.power();
    data.energy = pzem.energy();
    data.frequency = pzem.frequency();
    data.pf = pzem.pf();
    return data;
}

bool PowerSensor::resetEnergy()
{
    return pzem.resetEnergy();
}