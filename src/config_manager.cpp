#include "config_manager.h"

namespace {
constexpr uint32_t kConfigVersion = 1;
}

ConfigManager configManager;

void ConfigManager::begin() {
    prefs.begin("izar_cfg", false);

    if (!prefs.isKey("cfgVer") || prefs.getUInt("cfgVer", 0) != kConfigVersion) {
        applyDefaults();
        save();
    } else {
        load();
    }
}

Config& ConfigManager::getConfig() {
    return config;
}

void ConfigManager::save() {
    prefs.putUInt("cfgVer", kConfigVersion);

    prefs.putString("wifiSSID", config.wifiSSID);
    prefs.putString("wifiPass", config.wifiPassword);
    prefs.putString("hostname", config.hostname);

    prefs.putString("mqttBroker", config.mqttBroker);
    prefs.putUInt("mqttPort", config.mqttPort);
    prefs.putString("mqttUser", config.mqttUsername);
    prefs.putString("mqttPass", config.mqttPassword);
    prefs.putString("mqttClient", config.mqttClientId);
    prefs.putString("mqttBase", config.mqttBaseTopic);

    prefs.putString("serialNum", config.serialNumber);
}

void ConfigManager::reset() {
    prefs.clear();
    applyDefaults();
    save();
}

void ConfigManager::load() {
    copyString(config.wifiSSID, sizeof(config.wifiSSID), prefs.getString("wifiSSID", WIFI_SSID_DEFAULT));
    copyString(config.wifiPassword, sizeof(config.wifiPassword), prefs.getString("wifiPass", WIFI_PASSWORD_DEFAULT));
    copyString(config.hostname, sizeof(config.hostname), prefs.getString("hostname", WIFI_HOSTNAME_DEFAULT));

    copyString(config.mqttBroker, sizeof(config.mqttBroker), prefs.getString("mqttBroker", MQTT_BROKER_DEFAULT));
    config.mqttPort = static_cast<uint16_t>(prefs.getUInt("mqttPort", MQTT_PORT_DEFAULT));
    copyString(config.mqttUsername, sizeof(config.mqttUsername), prefs.getString("mqttUser", MQTT_USERNAME_DEFAULT));
    copyString(config.mqttPassword, sizeof(config.mqttPassword), prefs.getString("mqttPass", MQTT_PASSWORD_DEFAULT));
    copyString(config.mqttClientId, sizeof(config.mqttClientId), prefs.getString("mqttClient", MQTT_CLIENT_ID_DEFAULT));
    copyString(config.mqttBaseTopic, sizeof(config.mqttBaseTopic),
               prefs.getString("mqttBase", MQTT_BASE_TOPIC_DEFAULT));

    copyString(config.serialNumber, sizeof(config.serialNumber),
               prefs.getString("serialNum", IZAR_SERIAL_NUMBER_DEFAULT));
}

void ConfigManager::applyDefaults() {
    copyString(config.wifiSSID, sizeof(config.wifiSSID), WIFI_SSID_DEFAULT);
    copyString(config.wifiPassword, sizeof(config.wifiPassword), WIFI_PASSWORD_DEFAULT);
    copyString(config.hostname, sizeof(config.hostname), WIFI_HOSTNAME_DEFAULT);

    copyString(config.mqttBroker, sizeof(config.mqttBroker), MQTT_BROKER_DEFAULT);
    config.mqttPort = MQTT_PORT_DEFAULT;
    copyString(config.mqttUsername, sizeof(config.mqttUsername), MQTT_USERNAME_DEFAULT);
    copyString(config.mqttPassword, sizeof(config.mqttPassword), MQTT_PASSWORD_DEFAULT);
    copyString(config.mqttClientId, sizeof(config.mqttClientId), MQTT_CLIENT_ID_DEFAULT);
    copyString(config.mqttBaseTopic, sizeof(config.mqttBaseTopic), MQTT_BASE_TOPIC_DEFAULT);

    copyString(config.serialNumber, sizeof(config.serialNumber), IZAR_SERIAL_NUMBER_DEFAULT);
}

void ConfigManager::copyString(char* dest, size_t destSize, const String& src) {
    strlcpy(dest, src.c_str(), destSize);
}
