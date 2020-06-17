#pragma once
#include "shared.h"

class TemparatureControl
{
private:
    static unsigned long lastTempReading;
public:
    // Methods
    static void begin();
    static void handle();
    static float getCurrentTempF();
};

// Static member definitions
unsigned long TemparatureControl::lastTempReading = 0U;


// Public methods
void TemparatureControl::begin()
{
    sensors.setResolution(9);
    sensors.begin();
    sensors.requestTemperatures();
}

void TemparatureControl::handle()
{
    if (LOG_TEMPERATURE)
    {
        if (millis() - lastTempReading >= 10000)
        {
            LOG.print("Temperature Reading: ");
            LOG.println(getCurrentTempF());
            lastTempReading = millis();
        }
    }
}

float TemparatureControl::getCurrentTempF()
{
    sensors.requestTemperatures();
    return sensors.getTempFByIndex(0);
}