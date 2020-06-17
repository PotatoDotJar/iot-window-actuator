#pragma once
#include "shared.h"
#include "led_control.h"

class WiFiControl
{
private:

public:
    static void begin();
    static void handle();
};

void WiFiControl::begin()
{
    LOG.println("Setting up WiFi...");
    wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
        LOG.println("Entered config mode");
        LOG.println(WiFi.softAPIP());

        LOG.println(myWiFiManager->getConfigPortalSSID());
    });

    wifiManager.setSaveConfigCallback([]() {
        LOG.println("Should save config");

        // TODO: Save SSID and PASSWORD TO EEPROM
    });

    wifiManager.setConfigPortalTimeout(180);

    // Pull save data from EEPROM
    wifiManager.autoConnect(CLIENT_ID, AP_PASSWD);

    LedControl::setStatusLedColor(StatusColors::WIFI_CONNECTED);
    LOG.println("WiFi Setup complete!");
}

void WiFiControl::handle()
{
    // Nothing here right now
}