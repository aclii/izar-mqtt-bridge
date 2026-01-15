#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class WiFiManager {
  private:
    unsigned long lastConnectAttempt = 0;
    bool isConnected = false;
    bool apModeActive = false;
    bool mdnsStarted = false;
    String apSsid;

  public:
    WiFiManager();

    bool init();
    bool connect();
    bool isWiFiConnected();
    void reconnect();
    void handleWiFi();
    bool isApModeActive();

    int getRSSI(); // Signal strength

  private:
    void printWiFiStatus();
    void startAccessPoint();
    void startMdns();
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
