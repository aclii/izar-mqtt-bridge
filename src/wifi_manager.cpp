#include "wifi_manager.h"
#include "config_manager.h"
#include <ESPmDNS.h>

WiFiManager wifiManager;

WiFiManager::WiFiManager() {}

bool WiFiManager::init() {
    WiFi.mode(WIFI_STA);
    LOG_INFO("WiFi", "Initializing WiFi...");
    return true;
}

bool WiFiManager::connect() {
    if (apModeActive) {
        return false;
    }

    if (isConnected) {
        return true;
    }

    const Config& config = configManager.getConfig();
    if (strlen(config.wifiSSID) == 0 || strcmp(config.wifiSSID, WIFI_SSID_DEFAULT) == 0) {
        LOG_INFO("WiFi", "WiFi SSID not configured - starting AP for configuration");
        startAccessPoint();
        return false;
    }

    unsigned long now = millis();
    if (now - lastConnectAttempt < 5000) {
        return false; // Too soon to retry
    }

    lastConnectAttempt = now;

    if (strlen(config.hostname) > 0) {
        WiFi.setHostname(config.hostname);
    }

    LOG_INFO("WiFi", "Connecting to %s", config.wifiSSID);
    WiFi.begin(config.wifiSSID, config.wifiPassword);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECTION_TIMEOUT) {
        delay(500);
        LOG_DEBUG("WiFi", ".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        apModeActive = false;
        printWiFiStatus();
        startMdns();
        return true;
    }

    isConnected = false;
    LOG_ERROR("WiFi", "Connection failed");
    startAccessPoint();
    return false;
}

bool WiFiManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::reconnect() {
    if (apModeActive) {
        return;
    }

    if (!isWiFiConnected()) {
        connect();
    }
}

void WiFiManager::handleWiFi() {
    if (apModeActive) {
        return;
    }

    if (!isWiFiConnected()) {
        isConnected = false;
        reconnect();
    } else {
        isConnected = true;
        startMdns();
    }
}

bool WiFiManager::isApModeActive() {
    return apModeActive;
}

int WiFiManager::getRSSI() {
    return WiFi.RSSI();
}

void WiFiManager::printWiFiStatus() {
    LOG_INFO("WiFi", "Connected!");
    LOG_INFO("WiFi", "IP: %s", WiFi.localIP().toString().c_str());
    LOG_INFO("WiFi", "RSSI: %d dBm", WiFi.RSSI());
}

void WiFiManager::startAccessPoint() {
    if (apModeActive) {
        return;
    }

    WiFi.mode(WIFI_AP_STA);

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String ssid = String(AP_SSID_PREFIX_DEFAULT) + "-" + mac.substring(mac.length() - 4);
    apSsid = ssid;

    bool ok = WiFi.softAP(apSsid.c_str(), AP_PASSWORD_DEFAULT);
    if (ok) {
        apModeActive = true;
        LOG_INFO("WiFi", "AP started: %s", apSsid.c_str());
        LOG_INFO("WiFi", "AP IP: %s", WiFi.softAPIP().toString().c_str());
    } else {
        LOG_ERROR("WiFi", "Failed to start AP");
    }
}

void WiFiManager::startMdns() {
    if (mdnsStarted) {
        return;
    }

    const Config& config = configManager.getConfig();
    const char* hostname = strlen(config.hostname) > 0 ? config.hostname : WIFI_HOSTNAME_DEFAULT;
    if (MDNS.begin(hostname)) {
        mdnsStarted = true;
        LOG_INFO("WiFi", "mDNS responder started: %s.local", hostname);
    } else {
        LOG_WARN("WiFi", "Failed to start mDNS responder");
    }
}
