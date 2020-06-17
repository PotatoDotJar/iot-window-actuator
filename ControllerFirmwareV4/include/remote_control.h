#pragma once
#include "shared.h"
#include "motor_control.h"

class RemoteControl
{
private:
    static bool isClosing;
    static bool isOpening;

public:
    static void begin();
    static void handle();
};

// Static member definitions
bool RemoteControl::isClosing = false;
bool RemoteControl::isOpening = false;

// Public Methods
void RemoteControl::begin()
{
    openDebouncer.attach(MANUAL_OPEN_BUTTON, INPUT_PULLUP);
    openDebouncer.interval(25);

    closeDebouncer.attach(MANUAL_CLOSE_BUTTON, INPUT_PULLUP);
    closeDebouncer.interval(25);
}

void RemoteControl::handle()
{
    openDebouncer.update();
    closeDebouncer.update();

    // Handle Remote Window Open
    if (openDebouncer.fell())
    {
        if (!isClosing)
        {
            MotorControl::setRequestedMotorState(MotorState::OPENING);
            isOpening = true;
        }
    }

    if (openDebouncer.rose())
    {
        if (isOpening)
        {
            MotorControl::setRequestedMotorState(MotorState::STOPPED);
            isOpening = false;
        }
    }

    // Handle Remote Window Close
    if (closeDebouncer.fell())
    {
        if (!isOpening)
        {
            MotorControl::setRequestedMotorState(MotorState::CLOSING);
            isClosing = true;
        }
    }

    if (closeDebouncer.rose())
    {
        if (isClosing)
        {
            MotorControl::setRequestedMotorState(MotorState::STOPPED);
            isClosing = false;
        }
    }
}