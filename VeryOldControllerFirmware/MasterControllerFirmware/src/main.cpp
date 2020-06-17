/**
    Master Window Controller Firmware, main.cpp
    Purpose: Firmware with only an API IO and front-end interface.

    @author Richard Nader, Jr
    @version 3.5 10/25/18 
*/

#include <Arduino.h>
// Temperature
#include <OneWire.h>
#include <DallasTemperature.h>

// Networking
#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <HTTPClient.h>

// Motor
#include <AccelStepper.h>

// Base web html (All content is loaded remotly)
const char INDEX_HTML[] = R"======(
<html>
<head>
    <title>Window Slide Controller</title>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, user-scalable=no" />
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" />
    <link rel="stylesheet" href="//cdn.jsdelivr.net/npm/alertifyjs@1.11.1/build/css/alertify.min.css"/>
    <link rel="stylesheet" href="http://cdn.potatosaucevfx.com:9000/Public/style.css" />
    <link href="https://fonts.googleapis.com/icon?family=Material+Icons" rel="stylesheet">
</head>
<body>
    <h1 id="error-msg" style="visibility: hidden;">FAILED TO RETREVE CONTENT FROM CDN!!!!!</h1>

    <script src="//cdn.jsdelivr.net/npm/alertifyjs@1.11.1/build/alertify.min.js"></script>
    <script src="https://code.jquery.com/jquery-3.3.1.min.js"></script>
    <script>

        $.ajax({
            url: 'http://cdn.potatosaucevfx.com:9000/Public/index.html', 
            dataType: 'html', 
            success: function(response) {
                $("#error-msg").css('visibility','hidden');
                $('body').html(response); 
            },
            error: function(e) {
                console.error(e);
                $("#error-msg").css('visibility','visible');
                alertify.alert("Page Load Error", "Error Loading page from CDN!\n" + e);
            }
        });
    </script>
</body>
</html>

)======";

const char SETTINGS_HTML[] = R"======(
<html>
<head>
    <title>Window Slide Controller Settings</title>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, user-scalable=no" />
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" />
    <link rel="stylesheet" href="//cdn.jsdelivr.net/npm/alertifyjs@1.11.1/build/css/alertify.min.css"/>
    <link rel="stylesheet" href="http://cdn.potatosaucevfx.com:9000/Public/Settings/style.css" />
    <link href="https://fonts.googleapis.com/icon?family=Material+Icons" rel="stylesheet">
</head>
<body>
    <h1 id="error-msg" style="visibility: hidden;">FAILED TO RETREVE CONTENT FROM CDN!!!!!</h1>

    <script src="//cdn.jsdelivr.net/npm/alertifyjs@1.11.1/build/alertify.min.js"></script>
    <script src="https://code.jquery.com/jquery-3.3.1.min.js"></script>
    <script>
        $.ajax({
            url: 'http://cdn.potatosaucevfx.com:9000/Public/Settings/index.html', 
            dataType: 'html', 
            success: function(response) {
                $("#error-msg").css('visibility','hidden');
                $('body').html(response); 
            },
            error: function(e) {
                console.error(e);
                $("#error-msg").css('visibility','visible');
                alertify.alert("Page Load Error", "Error Loading page from CDN!\n" + e);
            } 
        });
    </script>
</body>
</html>

)======";

/*
    API ERROR CODES (eCode)
    17  -> Window moving
    7   -> Window already at that state
    77  -> Invalid Parameters
    770 -> API GET error
    700 -> Controller Busy
    0   -> OK
*/

// WIFI SETTINGS
const char *ssid = "MY_SSID";
const char *password = "MY_PASSWORD";

unsigned long restart_millis = 7200000;

IPAddress local_IP(192, 168, 1, 120);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int port = 80;
const char *hostname = "WindowController";
AsyncWebServer server(port); // Server init

// Temperature Settings
#define ONE_WIRE_BUS 14
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int SPS = 10000; // Steps per second
int MOTOR_ACCEL = 2000;

#define STEP_PIN 4
#define DIR_PIN 0
#define ENABLE_PIN 21
#define SLEEP_PIN 16
#define RST_PIN 17
#define MICRO_PIN_1 19
#define MICRO_PIN_2 18
#define MICRO_PIN_3 5
AccelStepper stepper(1, STEP_PIN, DIR_PIN);

// End Stops
#define OPEN_ENDSTOP_PIN 27
#define CLOSE_ENDSTOP_PIN 26

// Methods
bool isOpen();
bool isClosed();
void setStatusLed(bool state);
float getTempF();
float getTempC();
void enableStepper();
void disableStepper();
JsonObject &requestSlaveOpen(int index);
JsonObject &requestSlaveClose(int index);
JsonObject &getSlaveStatus(int index);
JsonObject &getSlaveWifiSignal(int index);
// End of Methods

// Global variables
boolean isWindowOpen = false; // Should be detected in setup
String slaveWindowIPs[] = {"192.168.1.121"};
boolean isSlaveWindowsOpen[] = {false};
boolean restartRequired = false;

// Movement states
bool isOpening = false;
bool isClosing = false;


// Master Json Buffer
DynamicJsonBuffer jsonBuffer;


void notFound(AsyncWebServerRequest *request)
{
    request->redirect("/");
}

void setup()
{
    // Local Serial Monitor
    Serial.begin(115200);

    // Temp sensors init
    sensors.begin();

    // Motor setup
    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(MICRO_PIN_1, OUTPUT);
    pinMode(MICRO_PIN_2, OUTPUT);
    pinMode(MICRO_PIN_3, OUTPUT);
    pinMode(SLEEP_PIN, OUTPUT);
    pinMode(RST_PIN, OUTPUT);

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

    Serial.println("Getting slave window status...");

    isSlaveWindowsOpen[0] = (getSlaveStatus(0)["state"] == "open");

    // Request home
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", INDEX_HTML);
    });

    // Request settings
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", SETTINGS_HTML);
    });

    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    },
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
                          restartRequired = true; // Tell the main loop to restart the ESP
                      }
                      else
                      {
                          Update.printError(Serial);
                      }
                      Serial.setDebugOutput(false);
                  }
              });

    // Request temperatures
    server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonObject &res = jsonBuffer.createObject();

        if (!isClosing && !isOpening)
        {
            res["eCode"] = 0;
            res["message"] = "Success";
            res["tempF"] = getTempF();
            res["tempC"] = getTempC();
        }
        else
        {
            res["eCode"] = 700;
            res["message"] = "Controller is busy...";
            res["tempF"] = -1;
            res["tempC"] = -1;
        }

        String str = "";
        res.printTo(str);

        request->send(200, "application/json", str);
    });

    // Request wifi strength
    server.on("/wifiSignal", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonArray &array = jsonBuffer.createArray();


        JsonObject &res = jsonBuffer.createObject();

        res["eCode"] = 0;
        res["message"] = "Success";
        res["windowID"] = 0;
        res["signal"] = WiFi.RSSI();

        array.add(res);

        JsonObject &slaveWindow = getSlaveWifiSignal(0);
        slaveWindow["windowID"] = 1;
        array.add(slaveWindow);


        String str = "";
        array.printTo(str);

        request->send(200, "application/json", str);
    });

    // Request temperatures
    server.on("/loadTemp", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonObject &res = jsonBuffer.createObject();

        res["eCode"] = 0;
        res["message"] = "Success";
        res["tempF"] = getTempF();
        res["tempC"] = getTempC();

        String str = "";
        res.printTo(str);

        request->send(200, "application/json", str);
    });

    server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonArray &array = jsonBuffer.createArray();

        // For master window
        JsonObject &windowObj = jsonBuffer.createObject();
        windowObj["windowID"] = 0;
        if (isWindowOpen)
        {
            windowObj["state"] = "open";
        }
        else
        {
            windowObj["state"] = "closed";
        }

        array.add(windowObj);

        // For slave windows
        JsonObject &slaveWindow = getSlaveStatus(0);
        slaveWindow["windowID"] = 1;
        array.add(slaveWindow);

        String str = "";
        array.printTo(str);
        request->send(200, "application/json", str);
    });

    server.on("/open", HTTP_POST, [](AsyncWebServerRequest *request) {
        //Serial.println("Requested open!");
        int id = 0;
        if (request->hasParam("id"))
        {
            id = request->getParam("id")->value().toInt();

            if (id == 0)
            {

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
            }
            else if (id != 0 && id <= sizeof(slaveWindowIPs))
            {
                String str = "";
                requestSlaveOpen(id).printTo(str);
                request->send(200, "application/json", str);
            }
            else
            {
                JsonObject &res = jsonBuffer.createObject();
                res["status"] = false;
                res["eCode"] = 77;
                res["message"] = "Invalid Parameter id!";
                String str = "";
                res.printTo(str);
                request->send(200, "application/json", str);
            }
        }
        else
        {
            JsonObject &res = jsonBuffer.createObject();
            res["status"] = false;
            res["eCode"] = 77;
            res["message"] = "Invalid Parameters! URL must contain a window ID.";
            String str = "";
            res.printTo(str);
            request->send(200, "application/json", str);
        }
    });

    server.on("/close", HTTP_POST, [](AsyncWebServerRequest *request) {
        //Serial.println("Requested close!");
        int id = 0;
        if (request->hasParam("id"))
        {
            id = request->getParam("id")->value().toInt();

            if (id == 0)
            {

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
            }
            else if (id != 0 && id <= sizeof(slaveWindowIPs))
            {
                String str = "";
                requestSlaveClose(id).printTo(str);
                request->send(200, "application/json", str);
            }
            else
            {
                JsonObject &res = jsonBuffer.createObject();
                res["status"] = false;
                res["eCode"] = 77;
                res["message"] = "Invalid Parameter id!";
                String str = "";
                res.printTo(str);
                request->send(200, "application/json", str);
            }
        }
        else
        {
            JsonObject &res = jsonBuffer.createObject();
            res["status"] = false;
            res["eCode"] = 77;
            res["message"] = "Invalid Parameters! URL must contain a window ID.";
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

float getTempF()
{
    sensors.requestTemperatures();

    while (sensors.getTempFByIndex(0) >= 150)
    {
        sensors.requestTemperatures();
    }

    return sensors.getTempFByIndex(0);
}

float getTempC()
{
    sensors.requestTemperatures();
    while (sensors.getTempCByIndex(0) >= 65.5556)
    {
        sensors.requestTemperatures();
    }
    return sensors.getTempCByIndex(0);
}

void enableStepper()
{
    digitalWrite(ENABLE_PIN, LOW);
}

void disableStepper()
{
    digitalWrite(ENABLE_PIN, HIGH);
}

JsonObject &requestSlaveOpen(int index)
{
    index -= 1;
    if (index < sizeof(slaveWindowIPs) && index >= 0)
    {
        HTTPClient http;
        http.begin("http://" + slaveWindowIPs[index] + "/open");

        int resCode = http.POST("");

        if (resCode > 0)
        {
            String response = http.getString();
            Serial.println("Open Response:\n" + response);

            JsonObject &res = jsonBuffer.parseObject(response);
            return res;
        }
        else
        {
            // Error
            Serial.println("Error opening slave window!\t Slave " + String(index) + "\t Error code: " + String(resCode));

            JsonObject &res = jsonBuffer.createObject();
            res["eCode"] = 770;
            res["message"] = "Error sending a request to slave " + String(index + 1) + "\n Error code: " + String(resCode);

            return res;
        }
        http.end();
    }
    else
    {
        Serial.println("Requested slave status index out of bounds!!!");
        JsonObject &res = jsonBuffer.createObject();
        res["eCode"] = 770;
        res["message"] = "Slave index is out of bounds!";

        return res;
    }
}

JsonObject &requestSlaveClose(int index)
{
    index -= 1;
    if (index < sizeof(slaveWindowIPs) && index >= 0)
    {
        HTTPClient http;
        http.begin("http://" + slaveWindowIPs[index] + "/close");
        int resCode = http.POST("");

        if (resCode > 0)
        {
            String response = http.getString();
            Serial.println("Close Response:\n" + response);

            JsonObject &res = jsonBuffer.parseObject(response);
            return res;
        }
        else
        {
            // Error
            Serial.println("Error closing slave window!\t Slave " + String(index) + "\t Error code: " + String(resCode));

            JsonObject &res = jsonBuffer.createObject();
            res["eCode"] = 770;
            res["message"] = "Error sending a request to slave " + String(index + 1) + "\n Error code: " + String(resCode);

            return res;
        }
        http.end();
    }
    else
    {
        Serial.println("Requested slave close index out of bounds!!!");
        JsonObject &res = jsonBuffer.createObject();
        res["eCode"] = 770;
        res["message"] = "Slave index is out of bounds!";

        return res;
    }
}

JsonObject &getSlaveStatus(int index)
{
    Serial.println("Getting status for slave " + index);

    if (index < sizeof(slaveWindowIPs) && index >= 0)
    {
        HTTPClient http;
        http.begin("http://" + slaveWindowIPs[index] + "/state");
        int resCode = http.GET();

        if (resCode > 0)
        {
            String response = http.getString();
            Serial.println("Status Response:\n" + response);
            JsonObject &res = jsonBuffer.parseObject(response);
            return res;
        }
        else
        {
            // Error
            Serial.println("Error getting slave status!\t Slave " + String(index) + "\t Error code: " + String(resCode));

            JsonObject &res = jsonBuffer.createObject();
            res["eCode"] = 770;
            res["message"] = "Error sending a request to slave " + String(index + 1) + "\n Error code: " + String(resCode);

            return res;
        }
        http.end();
    }
    else
    {
        Serial.println("Requested slave status index out of bounds!!!");
        JsonObject &res = jsonBuffer.createObject();
        res["eCode"] = 770;
        res["message"] = "Slave index is out of bounds!";

        return res;
    }
}

JsonObject &getSlaveWifiSignal(int index)
{
    Serial.println("Getting wifi signal for slave " + index);

    if (index < sizeof(slaveWindowIPs) && index >= 0)
    {
        HTTPClient http;
        http.begin("http://" + slaveWindowIPs[index] + "/wifiSignal");
        int resCode = http.GET();

        if (resCode > 0)
        {
            String response = http.getString();
            Serial.println("Signal Response:\n" + response);
            JsonObject &res = jsonBuffer.parseObject(response);
            return res;
        }
        else
        {
            // Error
            Serial.println("Error getting slave wifi signal!\t Slave " + String(index) + "\t Error code: " + String(resCode));

            JsonObject &res = jsonBuffer.createObject();
            res["eCode"] = 770;
            res["message"] = "Error sending a request to slave " + String(index + 1) + "\n Error code: " + String(resCode);

            return res;
        }
        http.end();
    }
    else
    {
        Serial.println("Requested slave wifi signal index out of bounds!!!");
        JsonObject &res = jsonBuffer.createObject();
        res["eCode"] = 770;
        res["message"] = "Slave index is out of bounds!";

        return res;
    }
}