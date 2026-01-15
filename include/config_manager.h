#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

struct Config {
    char wifiSSID[33];
    char wifiPassword[65];
    char hostname[33];

    char mqttBroker[65];
    uint16_t mqttPort;
    char mqttUsername[33];
    char mqttPassword[65];
    char mqttClientId[33];
    char mqttBaseTopic[65];

    char serialNumber[17];
};

class ConfigManager {
  public:
    void begin();
    Config& getConfig();
    void save();
    void reset();

  private:
    void load();
    void applyDefaults();
    void copyString(char* dest, size_t destSize, const String& src);

    Preferences prefs;
    Config config{};
};

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
