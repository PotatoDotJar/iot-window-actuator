#pragma once
#include "shared.h"
#include "motor_control.h"
#include "led_control.h"

class OtaHandler
{
private:
    /* data */
public:
    // Methods
    static void begin();
    static void handle();
};

void OtaHandler::begin()
{
    ArduinoOTA.setPort(3232);
    ArduinoOTA.setPassword(AP_PASSWD);

    char hostname[] = CLIENT_ID;
    ArduinoOTA.setHostname(hostname);

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            LedControl::setStatusLedColor(StatusColors::UPDATE_START);
            MotorControl::setCurrentWindowState(WindowState::UPDATING);
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            LedControl::setStatusLedColor(StatusColors::UPDATE_SUCCESS);
            MotorControl::setCurrentWindowState(WindowState::UPDATE_COMPLETE);
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error Updating OTA[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
            {
                Serial.println("Auth Failed");
            }
            else if (error == OTA_BEGIN_ERROR)
            {
                Serial.println("Begin Failed");
            }
            else if (error == OTA_CONNECT_ERROR)
            {
                Serial.println("Connect Failed");
            }
            else if (error == OTA_RECEIVE_ERROR)
            {
                Serial.println("Receive Failed");
            }
            else if (error == OTA_END_ERROR)
            {
                Serial.println("End Failed");
            }
            LedControl::setErrorHasOccured(true, ErrorCode::UPDATE_ERROR);
            LedControl::setBaseStatus();
        });

    ArduinoOTA.begin();
}

void OtaHandler::handle()
{
    if(!MotorControl::isMotorMoving())
    {
        ArduinoOTA.handle();
    }
}