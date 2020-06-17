/**
    Slave Window Controller Firmware, main.cpp
    Purpose: Firmware with only an API IO. Only recives commands from master.

    @author Richard Nader, Jr
    @version 1.0 10/25/18 
*/

#include <Arduino.h>

// Networking
#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>

// Motor
#include <AccelStepper.h>

/*
    API ERROR CODES (eCode)
    17  -> Window moving
    7   -> Window already at that state
    77  -> Invalid Parameters
    404 -> Endpoint not found
    700 -> Controller Busy
    0   -> OK
*/

// WIFI SETTINGS
const char *ssid = "MY_SSID";
const char *password = "MY_PASSWORD";

unsigned long restart_millis = 7200000;

IPAddress local_IP(192, 168, 1, 121); // Must be reserved through router DHCP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int port = 80;
const char *hostname = "Window2SlaveController";
AsyncWebServer server(port); // Server init

int SPS = 10000; // Steps per second
int MOTOR_ACCEL = 2000;

#define STEP_PIN 0
#define DIR_PIN 2
#define ENABLE_PIN 19
#define SLEEP_PIN 4
#define RST_PIN 16
#define MICRO_PIN_1 18
#define MICRO_PIN_2 5
#define MICRO_PIN_3 17
AccelStepper stepper(1, STEP_PIN, DIR_PIN);

// End Stops
#define OPEN_ENDSTOP_PIN 27
#define CLOSE_ENDSTOP_PIN 26

// Methods
bool isOpen();
bool isClosed();
void setStatusLed(bool state);
void enableStepper();
void disableStepper();
// End of Methods

// Global variables
boolean isWindowOpen = false; // Should be detected in setup
boolean restartRequired = false;

// Movement states
bool isOpening = false;
bool isClosing = false;


// Main Json buffer
DynamicJsonBuffer jsonBuffer;


void notFound(AsyncWebServerRequest *request)
{
    JsonObject &res = jsonBuffer.createObject();

    res["eCode"] = 404;
    res["message"] = "Endpoint not found";

    String str = "";
    res.printTo(str);

    request->send(200, "application/json", str);
}

void setup()
{
    // Local Serial Monitor
    Serial.begin(115200);

    // Motor setup
    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(MICRO_PIN_1, OUTPUT);
    pinMode(MICRO_PIN_2, OUTPUT);
    pinMode(MICRO_PIN_3, OUTPUT);
    pinMode(SLEEP_PIN, OUTPUT);
    pinMode(RST_PIN, OUTPUT);

    // Pin defaults
    digitalWrite(SLEEP_PIN, HIGH);
    digitalWrite(RST_PIN, HIGH);
    digitalWrite(MICRO_PIN_1, HIGH);
    digitalWrite(MICRO_PIN_2, HIGH);
    digitalWrite(MICRO_PIN_3, HIGH);

    stepper.setMaxSpeed(SPS);
    stepper.setSpeed(SPS);
    stepper.setAcceleration(MOTOR_ACCEL);
    disableStepper();

    // End stops init
    pinMode(OPEN_ENDSTOP_PIN, INPUT);
    pinMode(CLOSE_ENDSTOP_PIN, INPUT);

    // LED Setup
    pinMode(LED_BUILTIN, OUTPUT);

    // WIFI connection start
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    if (!WiFi.config(local_IP, gateway, subnet))
    {
        Serial.println("STA Failed to configure!");
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    WiFi.setHostname(hostname);

    boolean statusOn = true;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (statusOn)
        {
            statusOn = false;
            setStatusLed(true);
        }
        else
        {
            statusOn = true;
            setStatusLed(false);
        }
    }
    setStatusLed(true); // Connected status

    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (isOpen())
    {
        isWindowOpen = true;
    }
    else if (isClosed())
    {
        isWindowOpen = false;
    }
    else
    {
        enableStepper();
        Serial.println("Homing...");
        stepper.setSpeed(SPS);

        bool running = true;
        while (running)
        {

            stepper.runSpeed();

            if (isClosed())
            {
                stepper.stop();
                Serial.println("Homed!");
                disableStepper();
                running = false;
            }
        }
        Serial.println("Closed window!");
        isWindowOpen = false;
    }

    // Request slave info
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonObject &res = jsonBuffer.createObject();

        res["eCode"] = 0;
        res["message"] = "Success";
        res["isWindowOpen"] = isWindowOpen;

        String str = "";
        res.printTo(str);

        request->send(200, "application/json", str);
    });

    // For remote flashing firmware
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError())?"FAIL":"OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        restartRequired = true;  // Tell the main loop to restart the ESP
        request->send(response); },
              [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                  //Upload handler chunks in data

                  if (!index)
                  { // if index == 0 then this is the first frame of data
                      Serial.printf("UploadStart: %s\n", filename.c_str());
                      Serial.setDebugOutput(true);

                      // calculate sketch space required for the update
                      uint32_t maxSketchSpace = 0x140000;
                      if (!Update.begin(maxSketchSpace))
                      { //start with max available size
                          Update.printError(Serial);
                      }
                  }

                  //Write chunked data to the free sketch space
                  if (Update.write(data, len) != len)
                  {
                      Update.printError(Serial);
                  }

                  if (final)
                  { // if the final flag is set then this is the last frame of data
                      if (Update.end(true))
                      { //true to set the size to the current progress
                          Serial.printf("Update Success: %u B\nRebooting...\n", index + len);
                      }
                      else
                      {
                          Update.printError(Serial);
                      }
                      Serial.setDebugOutput(false);
                  }
              });

    // Request wifi strength
    server.on("/wifiSignal", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonObject &res = jsonBuffer.createObject();

        res["eCode"] = 0;
        res["message"] = "Success";
        res["signal"] = WiFi.RSSI();

        String str = "";
        res.printTo(str);

        request->send(200, "application/json", str);
    });


    // Request esp restart
    server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
        JsonObject &res = jsonBuffer.createObject();

        res["eCode"] = 0;
        res["message"] = "Success";

        String str = "";
        res.printTo(str);

        request->send(200, "application/json", str);
        restartRequired = true;
    });


    server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonObject &windowObj = jsonBuffer.createObject();
        if (isWindowOpen)
        {
            windowObj["state"] = "open";
        }
        else
        {
            windowObj["state"] = "closed";
        }

        String str = "";
        windowObj.printTo(str);
        request->send(200, "application/json", str);
    });

    server.on("/open", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isOpen() && !isOpening && !isClosing)
        {
            isOpening = true;

            JsonObject &res = jsonBuffer.createObject();
            res["status"] = true;
            res["eCode"] = 0;
            res["message"] = "Opening...";
            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
        else if (isOpening || isClosing)
        {
            JsonObject &res = jsonBuffer.createObject();
            res["status"] = false;
            res["eCode"] = 17;
            res["message"] = "Window is currently moving...";

            if (isOpening)
            {
                res["dir"] = "open";
            }
            else if (isClosing)
            {
                res["dir"] = "closed";
            }

            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
        else
        {
            JsonObject &res = jsonBuffer.createObject();
            res["status"] = false;
            res["eCode"] = 7;
            res["message"] = "Window is already open!";
            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
    });

    server.on("/close", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isClosed() && !isOpening && !isClosing)
        {
            isClosing = true;

            JsonObject &res = jsonBuffer.createObject();
            res["status"] = true;
            res["eCode"] = 0;
            res["message"] = "Closing...";
            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
        else if (isOpening || isClosing)
        {
            JsonObject &res = jsonBuffer.createObject();
            res["status"] = false;
            res["eCode"] = 17;
            res["message"] = "Window is currently moving...";
            if (isOpening)
            {
                res["dir"] = "open";
            }
            else if (isClosing)
            {
                res["dir"] = "closed";
            }
            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
        else
        {
            JsonObject &res = jsonBuffer.createObject();
            res["status"] = false;
            res["eCode"] = 7;
            res["message"] = "Window is already Closed!";
            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
    });

    server.onNotFound(notFound);
    server.begin();
}

bool triggered = false;
void loop()
{

    if (restartRequired)
    { // check the flag here to determine if a restart is required
        Serial.printf("Restarting ESP\n\r");
        restartRequired = false;
        ESP.restart();
    }
    else if (isOpening)
    {

        if (!triggered)
        {
            isWindowOpen = true;
            enableStepper();
            Serial.println("Moving...");
            stepper.setSpeed(-SPS);
            triggered = true;
        }

        stepper.runSpeed();

        if (isOpen())
        {
            stepper.stop();
            Serial.println("Stopping");
            disableStepper();
            isOpening = false;
        }
    }
    else if (isClosing)
    {

        if (!triggered)
        {
            isWindowOpen = false;
            enableStepper();
            Serial.println("Moving...");
            stepper.setSpeed(SPS);
            triggered = true;
        }

        stepper.runSpeed();

        if (isClosed())
        {
            stepper.stop();
            Serial.println("Stopping");
            disableStepper();
            isClosing = false;
        }
    }
    else
    {
        triggered = false;
    }


    // Reset every set interval
    if(millis() > restart_millis && !isClosing && !isOpening)
    {
        restartRequired = true;
    }

}

bool isOpen()
{
    return (digitalRead(OPEN_ENDSTOP_PIN) == HIGH);
}

bool isClosed()
{
    return (digitalRead(CLOSE_ENDSTOP_PIN) == HIGH);
}

void setStatusLed(bool state)
{
    digitalWrite(LED_BUILTIN, state);
}

void enableStepper()
{
    digitalWrite(ENABLE_PIN, LOW);
}

void disableStepper()
{
    digitalWrite(ENABLE_PIN, HIGH);
}