#pragma once
// Get variables from build flags defined in platformio.ini
#ifndef CLIENT_ID
#define CLIENT_ID "East_Window"
#endif

#ifndef AP_PASSWD
#define AP_PASSWD "AP_PASSWORD_HERE"
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION 20200607
#endif

// ----- Static Controller Settings -----

// Settings for led_control.h
#define RGB_LED_PIN 15
#define LED_DIM_DELAY 10000
#define LED_DIM_SPEED 5000 // Dim led in 5 seconds

// Settings for utility_functions.h
#define ROUTINE_RESTART_TIME 43200000

// Settings for temperature_control.h
#define ONE_WIRE_BUS 14
#define LOG_TEMPERATURE false

// Settings for motor_control.h
#define STEP_PIN 16
#define DIR_PIN 4
#define ENABLE_PIN 22
#define MICRO_PIN_1 21
#define MICRO_PIN_2 5
#define MICRO_PIN_3 17
#define OPEN_ENDSTOP_PIN 27
#define CLOSE_ENDSTOP_PIN 26

#define MAX_MOTOR_SPEED 10000
#define MOTOR_SPEED 9500
#define AUTO_CLOSE_ON_STARTUP false
#define MOTOR_RUN_TIMEOUT 15000 // Motor should not run for more than 15 seconds

// Settings for mqtt_control.h
//#define MQTT_SERVER_IP "192.168.1.18"
#define MQTT_SERVER_IP "SOME_DOTNET_CORE_WBB_API"
#define MQTT_SERVER_PORT 1883
#define MQTT_SERVER_PASSWORD "MY_MQTT_SERVER_PASSWORD"
String COMMAND_TOPIC = String(CLIENT_ID) + "/COMMAND";
String UPDATE_TOPIC = String(CLIENT_ID) + "/UPDATE";
String TEMP_REQUEST_TOPIC = "TEMP_REQUEST";
String STATE_TOPIC = "STATE";
String TEMP_TOPIC = "TEMP";
String FIRMWARE_VERSION_TOPIC = "FIRMWARE_VER";
#define MQTT_CONNECT_TRY_INTERVAL 25000
#define MQTT_TEMP_INTERVAL 60000

// Settings for remote_control.h
#define MANUAL_OPEN_BUTTON 32
#define MANUAL_CLOSE_BUTTON 33

// ----- END Static Controller Settings -----

// ----- Global Objects -----
#include <TelnetSpy.h>
TelnetSpy LOG;

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
WiFiManager wifiManager;

#include <AccelStepper.h>
AccelStepper stepper;

WiFiClient wifiClient;

#include <PubSubClient.h>
PubSubClient mqttClient(wifiClient);

#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#include <Bounce2.h>
Bounce openDebouncer;
Bounce closeDebouncer;

#include <ArduinoOTA.h>
#include <FastLED.h>
// ----- END Global Objects -----
