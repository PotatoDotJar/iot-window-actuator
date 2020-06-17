#pragma once
#include "shared.h"
#include "led_control.h"

enum class WindowState : uint8_t
{
    NONE = 0,
    CLOSING = 1,
    OPENING = 2,
    CLOSED = 3,
    OPEN = 4,
    CLOSING_ERROR = 5,
    OPENING_ERROR = 6,
    UPDATING = 7,
    UPDATE_COMPLETE = 8,
    RESTARTING = 9
};

enum class MotorState : uint8_t
{
    STOPPED = 0,
    CLOSING = 1,
    OPENING = 2
};

class MotorControl
{
private:
    static bool motorEnabled;
    static bool triggered;
    static unsigned long lastMovementStart;
    static MotorState requestedMotorState;
    static WindowState currentWindowState;

    static void InitialWindowSetup();
    static void HandleMotorState();
    static void enableStepper();            // Enable stepper power
    static void disableStepper();           // Disable stepper power
    static bool isOpenEndstopTriggered();   // Is the open endstop triggered
    static bool isClosedEndstopTriggered(); // Is the closed endstop triggered

    

public:
    static void begin();
    static void handle();

    static void (*onWindowStateChange)(WindowState *curWindowState);
    static void setWindowStateChangeCallback(void (*func)(WindowState *curWindowState));

    static bool isMotorMoving();
    static WindowState getCurrentWindowState();
    static MotorState getRequestedMotorState();
    static void setCurrentWindowState(WindowState newState);
    static void setRequestedMotorState(MotorState requestedState);
    static String getWindowStateString(WindowState state);
    static String getMotorStateString(MotorState state);
};


// Static member definitions
bool MotorControl::motorEnabled = false;
bool MotorControl::triggered = false;
unsigned long MotorControl::lastMovementStart = 0U;
MotorState MotorControl::requestedMotorState = MotorState::STOPPED;
WindowState MotorControl::currentWindowState = WindowState::NONE;

void (*MotorControl::onWindowStateChange)(WindowState *curWindowState) = NULL;

// Private methods
void MotorControl::InitialWindowSetup()
{
    if (isOpenEndstopTriggered())
    {
        setCurrentWindowState(WindowState::OPEN);
    }
    else if (isClosedEndstopTriggered())
    {
        setCurrentWindowState(WindowState::CLOSED);
    }
    else
    {
        if (AUTO_CLOSE_ON_STARTUP)
        {
            enableStepper();
            LOG.println("Initially closing window...");

            bool running = true;
            LedControl::setStatusLedColor(StatusColors::HOMING);
            lastMovementStart = millis();
            stepper.setSpeed(MOTOR_SPEED);
            while (running)
            {
                stepper.runSpeed();

                if (isClosedEndstopTriggered())
                {
                    stepper.stop();
                    LOG.println("Window initially closed.");
                    setCurrentWindowState(WindowState::CLOSED);
                    running = false;
                }

                if (millis() - lastMovementStart >= MOTOR_RUN_TIMEOUT)
                {
                    stepper.stop();
                    LOG.println("Endstop has not been reached yet, stopping. Check window.");
                    setCurrentWindowState(WindowState::CLOSING_ERROR);
                    running = false;
                }
            }

            disableStepper();
            LedControl::setBaseStatus();
        }
    }
}

void MotorControl::HandleMotorState()
{
    if (requestedMotorState == MotorState::CLOSING)
    {
        // Run this once
        if (!triggered)
        {
            enableStepper();
            LOG.println("Closing window...");
            setCurrentWindowState(WindowState::CLOSING);
            stepper.setSpeed(MOTOR_SPEED);
            LedControl::setStatusLedColor(StatusColors::CLOSING);
            LedControl::setLedDimTemp(false); // Temp disable led dimming
            lastMovementStart = millis();
            triggered = true;
        }

        // Runs while motor state is CLOSING
        stepper.runSpeed();

        // End
        if (isClosedEndstopTriggered())
        {
            LOG.println("Window closed.");
            setCurrentWindowState(WindowState::CLOSED);
            requestedMotorState = MotorState::STOPPED;
        }

        // End with error
        if (millis() - lastMovementStart >= MOTOR_RUN_TIMEOUT)
        {
            LOG.println("Endstop has not been reached yet, stopping. Check window.");
            LedControl::setErrorHasOccured(true, ErrorCode::MOTOR_ENDSTOP_ERROR);
            setCurrentWindowState(WindowState::CLOSING_ERROR);
            requestedMotorState = MotorState::STOPPED;
        }
    }
    else if (requestedMotorState == MotorState::OPENING)
    {
        // Run this once
        if (!triggered)
        {
            enableStepper();
            LOG.println("Opening window...");
            setCurrentWindowState(WindowState::OPENING);
            stepper.setSpeed(-MOTOR_SPEED);
            LedControl::setStatusLedColor(StatusColors::OPENING);
            LedControl::setLedDimTemp(false); // Temp disable led dimming
            lastMovementStart = millis();
            triggered = true;
        }

        // Runs while motor state is OPENING
        stepper.runSpeed();

        // End
        if (isOpenEndstopTriggered())
        {
            LOG.println("Window opened.");
            setCurrentWindowState(WindowState::OPEN);
            requestedMotorState = MotorState::STOPPED;
        }

        // End with error
        if (millis() - lastMovementStart >= MOTOR_RUN_TIMEOUT)
        {
            LOG.println("Endstop has not been reached yet, stopping. Check window.");
            LedControl::setErrorHasOccured(true, ErrorCode::MOTOR_ENDSTOP_ERROR);
            setCurrentWindowState(WindowState::OPENING_ERROR);
            requestedMotorState = MotorState::STOPPED;
        }
    }
    else
    {
        // If stopped or anything else
        if (triggered)
        {
            triggered = false;
            disableStepper();
            stepper.stop();
            
            if (currentWindowState != WindowState::CLOSED && currentWindowState != WindowState::OPEN && currentWindowState != WindowState::CLOSING_ERROR && currentWindowState != WindowState::OPENING_ERROR)
            {
                setCurrentWindowState(WindowState::NONE);
            }

            LedControl::setBaseStatus();
            LedControl::setLedDimTemp(true); // Re-enable led dimming
        }
    }
}

void MotorControl::enableStepper()
{
    digitalWrite(ENABLE_PIN, LOW);
    motorEnabled = true;
}

void MotorControl::disableStepper()
{
    digitalWrite(ENABLE_PIN, HIGH);
    motorEnabled = false;
}

bool MotorControl::isOpenEndstopTriggered()
{
    return !digitalRead(OPEN_ENDSTOP_PIN);
}

bool MotorControl::isClosedEndstopTriggered()
{
    return !digitalRead(CLOSE_ENDSTOP_PIN);
}

void MotorControl::setCurrentWindowState(WindowState newState)
{
    currentWindowState = newState;

    // State change function callback for mqtt
    if (onWindowStateChange != NULL)
    {
        onWindowStateChange(&currentWindowState);
    }

    LOG.print("New window state is ");
    LOG.println(getWindowStateString(currentWindowState));
}

// Public methods
void MotorControl::begin()
{
    pinMode(ENABLE_PIN, OUTPUT);

    pinMode(MICRO_PIN_1, OUTPUT);
    digitalWrite(MICRO_PIN_1, HIGH);

    pinMode(MICRO_PIN_2, OUTPUT);
    digitalWrite(MICRO_PIN_2, HIGH);

    pinMode(MICRO_PIN_3, OUTPUT);
    digitalWrite(MICRO_PIN_3, LOW);

    pinMode(OPEN_ENDSTOP_PIN, INPUT_PULLUP);
    pinMode(CLOSE_ENDSTOP_PIN, INPUT_PULLUP);

    // Initialize motor
    disableStepper();
    stepper = AccelStepper(1, STEP_PIN, DIR_PIN);

    stepper.setMaxSpeed(MAX_MOTOR_SPEED);
    stepper.setSpeed(MOTOR_SPEED);

    InitialWindowSetup();
}

void MotorControl::handle()
{
    HandleMotorState();
}

 void MotorControl::setWindowStateChangeCallback(void (*func)(WindowState *curWindowState))
 {
     onWindowStateChange = func;
 }

bool MotorControl::isMotorMoving()
{
    return (currentWindowState == WindowState::CLOSING || currentWindowState == WindowState::OPENING);
}

MotorState MotorControl::getRequestedMotorState()
{
    return requestedMotorState;
}

WindowState MotorControl::getCurrentWindowState()
{
    return currentWindowState;
}

void MotorControl::setRequestedMotorState(MotorState requestedState)
{
    LOG.print("Requested motor state is ");
    LOG.println(getMotorStateString(requestedState));
    if (requestedState == MotorState::OPENING)
    {
        // Check if window is already open
        if (!isOpenEndstopTriggered())
        {
            requestedMotorState = requestedState;
        }
        else
        {
            LOG.println("Window is already open.");
            requestedMotorState = MotorState::STOPPED;
            setCurrentWindowState(WindowState::OPEN);
        }
    }
    else if (requestedState == MotorState::CLOSING)
    {
        // Check if window is already closed
        if (!isClosedEndstopTriggered())
        {
            requestedMotorState = requestedState;
        }
        else
        {
            LOG.println("Window is already closed.");
            requestedMotorState = MotorState::STOPPED;
            setCurrentWindowState(WindowState::CLOSED);
        }
    }
    else
    {
        LOG.println("Stopping window.");
        requestedMotorState = MotorState::STOPPED;
    }
}

String MotorControl::getWindowStateString(WindowState state)
{
    switch (state)
    {
    case WindowState::NONE:
        return String("NONE");
        break;

    case WindowState::CLOSING:
        return String("CLOSING");
        break;

    case WindowState::OPENING:
        return String("OPENING");
        break;

    case WindowState::CLOSED:
        return String("CLOSED");
        break;

    case WindowState::OPEN:
        return String("OPEN");
        break;

    case WindowState::CLOSING_ERROR:
        return String("CLOSING_ERROR");
        break;

    case WindowState::OPENING_ERROR:
        return String("OPENING_ERROR");
        break;

    case WindowState::UPDATING:
        return String("UPDATING");
        break;

    case WindowState::UPDATE_COMPLETE:
        return String("UPDATE_COMPLETE");
        break;

    case WindowState::RESTARTING:
        return String("RESTARTING");
        break;

    default:
        return String("UNKNOWN");
        break;
    }
}

String MotorControl::getMotorStateString(MotorState state)
{
    switch (state)
    {
    case MotorState::CLOSING:
        return String("CLOSING");
        break;

    case MotorState::OPENING:
        return String("OPENING");
        break;

    case MotorState::STOPPED:
        return String("STOPPED");
        break;

    default:
        return String("UNKNOWN");
        break;
    }
}