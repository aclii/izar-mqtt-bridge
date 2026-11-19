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
    if (isWiFiConnected()) {
        if (!isConnected) {
            isConnected = true;
            connectionInProgress = false;
            connectAttempted = false;
            printWiFiStatus();
            startMdns();
        }
        if (apModeActive && WiFi.softAPdisconnect(true)) {
            apModeActive = false;
            LOG_INFO("WiFi", "Configuration AP stopped");
        }
        return true;
    }

    if (isConnected) {
        isConnected = false;
        if (mdnsStarted) {
            MDNS.end();
            mdnsStarted = false;
        }
        LOG_WARN("WiFi", "Connection lost");
    }

    const Config& config = configManager.getConfig();
    if (strlen(config.wifiSSID) == 0 || strcmp(config.wifiSSID, WIFI_SSID_DEFAULT) == 0) {
        if (!apModeActive) {
            LOG_INFO("WiFi", "WiFi SSID not configured - starting AP for configuration");
        }
        startAccessPoint();
        return false;
    }

    unsigned long now = millis();
    if (connectionInProgress) {
        if (now - lastConnectAttempt < WIFI_CONNECTION_TIMEOUT) {
            return false;
        }
        connectionInProgress = false;
        lastConnectAttempt = now;
        WiFi.disconnect(false);
        LOG_ERROR("WiFi", "Connection timed out; retrying in %lu seconds", WIFI_RECONNECT_INTERVAL / 1000UL);
        startAccessPoint();
        return false;
    }

    if (connectAttempted && now - lastConnectAttempt < WIFI_RECONNECT_INTERVAL) {
        return false;
    }

    lastConnectAttempt = now;
    connectAttempted = true;
    connectionInProgress = true;

    if (strlen(config.hostname) > 0) {
        WiFi.setHostname(config.hostname);
    }

    LOG_INFO("WiFi", "Connecting to %s", config.wifiSSID);
    if (WiFi.begin(config.wifiSSID, config.wifiPassword) == WL_CONNECT_FAILED) {
        connectionInProgress = false;
        lastConnectAttempt = millis();
        LOG_ERROR("WiFi", "Could not start connection; retrying in %lu seconds", WIFI_RECONNECT_INTERVAL / 1000UL);
        startAccessPoint();
    }
    return false;
}

bool WiFiManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::handleWiFi() {
    connect();
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
