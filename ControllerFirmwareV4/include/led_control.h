#pragma once
#include "shared.h"

// Status color definitions
enum class StatusColors : uint8_t
{
    OPENING = 0,
    CLOSING = 1,
    HOMING = 2,
    WIFI_CONNECTED = 3,
    WIFI_DISCONNECTED = 4,
    MQTT_IS_CONNECTED = 5,
    MQTT_CONNNECTING = 6,
    UPDATE_START = 7,
    UPDATE_SUCCESS = 8,
    RESTARTING = 9,
    ERROR = 10,
    OFF = 11
};

// Error event definitions
enum class ErrorCode : uint8_t
{
    NONE = 0,
    MOTOR_ENDSTOP_ERROR = 1,
    UPDATE_ERROR = 2
};

class LedControl
{
private:
    static CRGB statusLedColor[1];

    // Led dimming state variables
    static bool enableLedDim;
    static bool tempLedDim;
    static bool requestLedDim;
    static bool ledDimmed;
    static unsigned long lastLEDWakeTime;
    static unsigned long lastLEDDimTime;

    // Error state variables
    static bool errorHasOccured;
    static ErrorCode errorCode;

public:
    // Methods
    static void setStatusLedColor(CRGB newColor, bool dimLed = true);
    static void setStatusLedColor(StatusColors newColor, bool dimLed = true);
    static CRGB getStatusLedColor();
    static void setBaseStatus();

    static void setLedDimTemp(bool shouldDim);

    static void setErrorHasOccured(bool hasErrorOccured, ErrorCode newErrorCode = ErrorCode::NONE);
    static bool hasErrorOccured();
    static ErrorCode getErrorCode();
    static String getErrorCodeString(ErrorCode errorCode);

    static void begin();
    static void handle();
};

// Static member definitions
CRGB LedControl::statusLedColor[1] = { CRGB::Black };
bool LedControl::enableLedDim = false;
bool LedControl::tempLedDim = false;
bool LedControl::requestLedDim = false;
bool LedControl::ledDimmed = false;
unsigned long LedControl::lastLEDWakeTime = 0U;
unsigned long LedControl::lastLEDDimTime = 0U;
bool LedControl::errorHasOccured = false;
ErrorCode LedControl::errorCode = ErrorCode::NONE;


// Public methods
void LedControl::setStatusLedColor(CRGB newColor, bool dimLed)
{
    statusLedColor[0] = newColor;
    FastLED.setBrightness(255);
    FastLED.show();

    requestLedDim = false;
    ledDimmed = false;

    if (dimLed)
    {
        enableLedDim = true;
        lastLEDWakeTime = millis();
    }
    else
    {
        enableLedDim = false;
    }
}

void LedControl::setStatusLedColor(StatusColors newColor, bool dimLed)
{

    switch (newColor)
    {
    case StatusColors::OPENING:
        statusLedColor[0] = CRGB::Green;
        break;
    case StatusColors::CLOSING:
        statusLedColor[0] = CRGB::Yellow;
        break;
    case StatusColors::HOMING:
        statusLedColor[0] = CRGB::Orange;
        break;
    case StatusColors::WIFI_CONNECTED:
        statusLedColor[0] = CRGB::Blue;
        break;
    case StatusColors::WIFI_DISCONNECTED:
        statusLedColor[0] = CRGB::Brown;
        break;
    case StatusColors::MQTT_IS_CONNECTED:
        statusLedColor[0] = CRGB::Purple;
        break;
    case StatusColors::MQTT_CONNNECTING:
        statusLedColor[0] = CRGB::Turquoise;
        break;
    case StatusColors::UPDATE_START:
        statusLedColor[0] = CRGB::GreenYellow;
        break;
    case StatusColors::UPDATE_SUCCESS:
        statusLedColor[0] = CRGB::DarkGreen;
        break;
    case StatusColors::RESTARTING:
        statusLedColor[0] = CRGB::GreenYellow;
        break;
    case StatusColors::ERROR:
        statusLedColor[0] = CRGB::IndianRed;
        break;
    case StatusColors::OFF:
        statusLedColor[0] = CRGB::Black;
        break;
    default:
        break;
    }

    setStatusLedColor(statusLedColor[0], dimLed);
}

CRGB LedControl::getStatusLedColor()
{
    return statusLedColor[0];
}

void LedControl::setBaseStatus()
{
    if (errorHasOccured)
    {
        setStatusLedColor(StatusColors::ERROR);
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        if (mqttClient.connected())
        {
            setStatusLedColor(StatusColors::MQTT_IS_CONNECTED);
        }
        else
        {
            setStatusLedColor(StatusColors::WIFI_CONNECTED);
        }
    }
    else
    {
        setStatusLedColor(StatusColors::WIFI_DISCONNECTED);
    }
}

void LedControl::setLedDimTemp(bool shouldDim)
{
    tempLedDim = shouldDim;
}

void LedControl::setErrorHasOccured(bool hasErrorOccured, ErrorCode newErrorCode)
{
    errorHasOccured = hasErrorOccured;

    errorCode = newErrorCode;
}

bool LedControl::hasErrorOccured()
{
    return errorHasOccured;
}

ErrorCode LedControl::getErrorCode()
{
    return errorCode;
}

String LedControl::getErrorCodeString(ErrorCode errorCode)
{
    switch (errorCode)
    {
    case ErrorCode::NONE:
        return String("NONE");
        break;

    case ErrorCode::MOTOR_ENDSTOP_ERROR:
        return String("MOTOR_ENDSTOP_ERROR");
        break;

    case ErrorCode::UPDATE_ERROR:
        return String("UPDATE_ERROR");
        break;

    default:
        return String("UNKNOWN");
        break;
    }
}

void LedControl::begin()
{
    LOG.println("Setting up led control");

    // Init static variables
    enableLedDim = true;
    tempLedDim = true;
    requestLedDim = false;
    ledDimmed = false;

    errorHasOccured = false;

    FastLED.addLeds<WS2812B, RGB_LED_PIN, GRB>(statusLedColor, 1);
    FastLED.setBrightness(255);
    setStatusLedColor(StatusColors::OFF);
    FastLED.show(); // Write data to led
}

void LedControl::handle()
{
    if (enableLedDim && tempLedDim)
    {
        // Handle led dim timeout
        if (!ledDimmed && !requestLedDim)
        {
            // Wait for timeout to dim led
            if (millis() - lastLEDWakeTime >= LED_DIM_DELAY)
            {
                //LOG.println("LED Dim triggered");
                requestLedDim = true;
                lastLEDDimTime = millis();
            }
        }
        else
        {
            if (!ledDimmed)
            {
                // Non-blocking led dimming
                unsigned long timeDifference = millis() - lastLEDDimTime;
                uint8_t calculatedVal = map(timeDifference, 0, LED_DIM_SPEED, 255, 1);

                //LOG.printf("LED Dim Debug: dif=%i val=%i\r", timeDifference, calculatedVal);

                FastLED.setBrightness(calculatedVal);
                FastLED.show();

                if (timeDifference >= LED_DIM_SPEED)
                {
                    // Led Dim complete
                    //LOG.printf("\nLED Dim Complete!");
                    ledDimmed = true;
                    requestLedDim = false;
                    lastLEDDimTime = 0;
                }
            }
        }
    }
}