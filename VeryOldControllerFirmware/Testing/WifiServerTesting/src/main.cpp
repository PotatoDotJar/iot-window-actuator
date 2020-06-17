#include <Arduino.h>

// Networking
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "MY_SSID";
const char *password = "MY_PASSWORD";
const char *hostname = "ESP32";

IPAddress local_IP(192, 168, 1, 121); // Must be reserved through router DHCP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const int port = 80;

AsyncWebServer server(port);

String header;
int val = 0;

// Base web html (All content is loaded remotly)
const char INDEX_HTML[] = R"======(
<html>
<head>
    <title>ESP32 Test Server</title>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css" crossorigin="anonymous">
    <style>
      
    </style>
</head>
<body>
  <main role="main">
    <div class="container">
      <h1 class="text-center">%val%</h1>
    </div>
  </main>

  
  <script src="https://code.jquery.com/jquery-3.3.1.slim.min.js" crossorigin="anonymous"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js" crossorigin="anonymous"></script>
  <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/js/bootstrap.min.js" crossorigin="anonymous"></script>
</body>
</html>

)======";

void notFound(AsyncWebServerRequest *request)
{
  request->redirect("/");
}

String processor(const String& _var)
{
  if(_var == "val")
    return F(val);
  return String();
}



void setup()
{

  Serial.begin(115200);

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
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      statusOn = true;
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Request home
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    val++;
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", INDEX_HTML, processor);
    request->send(response);
  });

  server.on("/value", HTTP_GET, [](AsyncWebServerRequest *request) {
    int pin = 0;
    DynamicJsonBuffer jsonBufferOut;
    JsonObject &res = jsonBufferOut.createObject();

    if (request->hasParam("pin"))
    {
      pin = request->getParam("pin")->value().toInt();
      res["value"] = digitalRead(pin);
    }
    else
    {
      res["error"] = "Endpoint requires pin number as a parameter!";
    }

    String str = "";
    res.printTo(str);
    request->send(200, "application/json", str);
  });

  server.onNotFound(notFound);
  server.begin();
}

void loop()
{
}