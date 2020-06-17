#include <Arduino.h>

// Classes
#include "shared.h"
#include "led_control.h"

// Temp feature
#ifdef ENABLE_TEMP_FEATURE
#include "temperature_control.h"
#endif

#include "wifi_control.h"
#include "motor_control.h"
#include "utility_functions.h"
#include "ota_control.h"
#include "remote_control.h"
#include "mqtt_control.h"

void setup()
{
	// Setup logging
	String welcomeMessage = "Connected to " + String(CLIENT_ID) + "\r\nOpen=O, Close=C, Stop=S, Check Error=E, Clear Error=X, WiFiSignal=W, Restart=R, Cur Temp=T\r\n";
	LOG.setWelcomeMsg((char *)welcomeMessage.c_str());
	LOG.begin(115200);

	LedControl::begin();

#ifdef ENABLE_TEMP_FEATURE
	TemparatureControl::begin();
#endif

	WiFiControl::begin();
	MotorControl::begin();
	Utilities::begin();
	OtaHandler::begin();
	RemoteControl::begin();
	MqttControl::begin();
}

void loop()
{
	LedControl::handle();

#ifdef ENABLE_TEMP_FEATURE
	TemparatureControl::handle();
#endif

	WiFiControl::handle();
	MotorControl::handle();
	Utilities::handle();
	OtaHandler::handle();
	RemoteControl::handle();
	MqttControl::handle();


	// Don't handle Telnet logging while motor is running,
    // this will cause the motor to stall under load.
	if (!MotorControl::isMotorMoving())
	{
		LOG.handle();
	}

	// Handle Telnet commands for testing
	if (LOG.available() > 0)
	{
		char command = LOG.read();

		if (command == 'O')
		{
			MotorControl::setRequestedMotorState(MotorState::OPENING);
		}
		else if (command == 'C')
		{
			MotorControl::setRequestedMotorState(MotorState::CLOSING);
		}
		else if (command == 'S')
		{
			MotorControl::setRequestedMotorState(MotorState::STOPPED);
		}
		else if (command == 'E')
		{
			if (LedControl::hasErrorOccured())
			{
				LOG.print("Last error code is ");
				LOG.println(LedControl::getErrorCodeString(LedControl::getErrorCode()));
			}
			else
			{
				LOG.println("No recent errors have occured.");
			}
		}
		else if (command == 'X')
		{
			LOG.println("Clearing errors.");
			LedControl::setErrorHasOccured(false);
			LedControl::setBaseStatus();
		}
		else if (command == 'W')
		{
			LOG.print("WiFi Signal Strength is ");
			LOG.println(WiFi.RSSI());
		}
		else if (command == 'R')
		{
			Utilities::restartController();
		}
		else if (command == 'T')
		{
#ifdef ENABLE_TEMP_FEATURE
			LOG.print("Temperature F: ");
			LOG.println(TemparatureControl::getCurrentTempF());
#else
			LOG.println("Temp feature not supported on this controller.");
#endif
		}
	}
}