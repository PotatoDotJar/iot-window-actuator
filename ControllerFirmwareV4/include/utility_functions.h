#pragma once
#include "shared.h"
#include "motor_control.h"

class Utilities
{
private:
    static bool restartTriggered;
    static unsigned long restartTriggerTime;
    static unsigned long restartDelay;

public:
    // Methods
    static void restartController(unsigned long delay = 3000U);
    static void begin();
    static void handle();
};

// Static member definitions
bool Utilities::restartTriggered = false;
unsigned long Utilities::restartTriggerTime = 0U;
unsigned long Utilities::restartDelay = 0U;

// Public methods
void Utilities::restartController(unsigned long delay)
{
    LOG.printf("Restart triggered, the controller is scheduled to restart in %F seconds.\n", (float)delay / 1000);
    restartTriggerTime = millis();
    restartDelay = delay;
    restartTriggered = true;

    MotorControl::setCurrentWindowState(WindowState::RESTARTING);
}

void Utilities::begin()
{
    // Do nothing...
}

void Utilities::handle()
{
    // Requested restart handler
    if (restartTriggered)
    {
        if (millis() - restartTriggerTime >= restartDelay)
        {
            LOG.println("Restarting now!");
            LOG.flush();
            ESP.restart();
        }
    }

    // Routine restart handler
    if (millis() >= ROUTINE_RESTART_TIME)
    {
        restartController();
    }
}