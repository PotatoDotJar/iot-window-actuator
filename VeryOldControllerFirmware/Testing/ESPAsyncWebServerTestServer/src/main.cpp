//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

const char *ssid = "MY_SSID";
const char *password = "MY_PASSWORD";

bool open();
bool close();

bool isOpen = false;
bool isOpening = false;
bool isClosing = false;
long lastTime = 0;

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void setup()
{

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("ESP32 1");
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world");
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {

        if(isOpen) {
            request->send(200, "text/plain", "The window is open.");
        }
        else 
        {
            request->send(200, "text/plain", "The window is closed.");
        }
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/open", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (open())
        {
            request->send(200, "text/plain", "Opening...");
        }
        else
        {
            request->send(200, "text/plain", "Error Opening!");
        }
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/close", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (close())
        {
            request->send(200, "text/plain", "Closing...");
        }
        else
        {
            request->send(200, "text/plain", "Error Closing!");
        }
    });

    server.onNotFound(notFound);

    server.begin();
}

bool open()
{
    if (!isOpening && !isClosing && !isOpen)
    {
        Serial.println("Opening...");
        lastTime = millis();
        isClosing = false;
        isOpening = true;
        return true;
    }
    else
    {
        Serial.println("Error Opening...");
        return false;
    }
}

bool close()
{
    if (!isOpening && !isClosing && isOpen)
    {
        Serial.println("Closing...");
        lastTime = millis();
        isOpening = false;
        isClosing = true;
        return true;
    }
    else
    {
        Serial.println("Error Closing...");
        return false;
    }
}

void loop()
{
    if (isOpening)
    {
        if (millis() - lastTime >= 10000)
        {
            isOpening = false;
            isOpen = true;
            Serial.println("Opened!");
        }
    }
    else if (isClosing)
    {
        if (millis() - lastTime >= 10000)
        {
            isClosing = false;
            isOpen = false;
            Serial.println("Closed!");
        }
    }
}