#pragma once
#include "shared.h"
#include "led_control.h"
#include "temperature_control.h"
#include "motor_control.h"

class MqttControl
{
private:
    // Internal static methods
    static void onMessageRecived(char *topic, byte *message, unsigned int length);
    static void registerSubscriptions();
    static void connect();

    // Handing setup
    static bool needsInit;
    static unsigned long lastConnectTryTime;

    // Periodic temperature updates
    static unsigned long lastTempSend;

    static void onWindowStateChanged(WindowState *curWindowState);

public:
    // Public static methods
    static void begin();
    static void handle();
    static void notifyStateUpdate(String newState);
};

// Static member definitions
bool MqttControl::needsInit = true;
unsigned long MqttControl::lastConnectTryTime = 0U;
unsigned long MqttControl::lastTempSend = 0U;

// Private methods
void MqttControl::onMessageRecived(char *topic, byte *message, unsigned int length)
{
    String response;

    String _topic = String(topic);

    for (int i = 0; i < length; i++)
    {
        response += (char)message[i];
    }

    LOG.print("Message arrived [");
    LOG.print(topic);
    LOG.print("] ");
    LOG.println(response);

    if (_topic.equals(COMMAND_TOPIC))
    {
        // The response is a command

        if (response == "OPEN")
        {
            MotorControl::setRequestedMotorState(MotorState::OPENING);
        }
        else if (response == "CLOSE")
        {
            MotorControl::setRequestedMotorState(MotorState::CLOSING);
        }
        else if (response == "STOP")
        {
            MotorControl::setRequestedMotorState(MotorState::STOPPED);
        }
        else if (response == "RESTART")
        {
            Utilities::restartController();
        }
        else
        {
            // Do nothing
        }
    }
    else if (_topic.equals(TEMP_REQUEST_TOPIC))
    {
        if (response == "TEMP")
        {
            mqttClient.publish(TEMP_TOPIC.c_str(), String(TemparatureControl::getCurrentTempF()).c_str());
        }
    }
    else
    {
        // Invalid topic
    }
}

void MqttControl::registerSubscriptions()
{
    mqttClient.subscribe(COMMAND_TOPIC.c_str());
    mqttClient.subscribe(UPDATE_TOPIC.c_str());
    #ifdef ENABLE_TEMP_FEATURE
    mqttClient.subscribe(TEMP_REQUEST_TOPIC.c_str());
    #endif
}

void MqttControl::connect()
{
    LOG.println("Connecting to MQTT Server...");

    LedControl::setStatusLedColor(StatusColors::MQTT_CONNNECTING, false);

    needsInit = true;

    while (!mqttClient.connected())
    {
        LOG.println("Attempting MQTT connection...");

        if (mqttClient.connect(CLIENT_ID, CLIENT_ID, MQTT_SERVER_PASSWORD))
        {
            registerSubscriptions();
            LOG.print("Connected to ");
            LOG.print(MQTT_SERVER_IP);
            LOG.println(".");
            LedControl::setBaseStatus();
        }
        else
        {
            LOG.println("Failed to connect to MQTT Server.");
            LedControl::setBaseStatus();
            break;
        }
    }
}

void MqttControl::onWindowStateChanged(WindowState *curWindowState)
{
    WindowState newState = *curWindowState;

    if (mqttClient.connected())
    {
        notifyStateUpdate(MotorControl::getWindowStateString(newState));
    }
}

// Public methods
void MqttControl::begin()
{
    LOG.println("Setting up MQTT...");

    MotorControl::setWindowStateChangeCallback(onWindowStateChanged);

    needsInit = true;

    mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
    mqttClient.setCallback(onMessageRecived);

    connect();
}

void MqttControl::handle()
{
    if (!mqttClient.connected())
    {
        if (!MotorControl::isMotorMoving())
        {
            // Only run if motor is not moving..
            if (millis() - lastConnectTryTime >= MQTT_CONNECT_TRY_INTERVAL)
            {
                connect();
                lastConnectTryTime = millis();
            }
        }
    }
    else
    {

        if (!MotorControl::isMotorMoving())
        {
// Only run if motor is not moving..
#ifdef ENABLE_TEMP_FEATURE
            // Periodic temp sending
            if (millis() - lastTempSend >= MQTT_TEMP_INTERVAL && !MotorControl::isMotorMoving())
            {
                mqttClient.publish(TEMP_TOPIC.c_str(), String(TemparatureControl::getCurrentTempF()).c_str());
                lastTempSend = millis();
            }
#endif

            // Send info about controller to server
            if (needsInit && !MotorControl::isMotorMoving())
            {
                mqttClient.publish(STATE_TOPIC.c_str(), MotorControl::getWindowStateString(MotorControl::getCurrentWindowState()).c_str());
                #ifdef ENABLE_TEMP_FEATURE
                mqttClient.publish(TEMP_TOPIC.c_str(), String(TemparatureControl::getCurrentTempF()).c_str());
                #endif
                mqttClient.publish(FIRMWARE_VERSION_TOPIC.c_str(), String(FIRMWARE_VERSION).c_str());
                needsInit = false;
            }
        }
    }

    mqttClient.loop();
}

void MqttControl::notifyStateUpdate(String newState)
{
    mqttClient.publish(STATE_TOPIC.c_str(), newState.c_str());
}
