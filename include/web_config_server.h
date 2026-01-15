#ifndef WEB_CONFIG_SERVER_H
#define WEB_CONFIG_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config_manager.h"
#include "config.h"

class WebConfigServer {
  public:
    explicit WebConfigServer(ConfigManager* cm);
    void begin();
    void handle();
    void stop();

  private:
    AsyncWebServer server;
    ConfigManager* configManager;
    bool dnsStarted = false;

    void setupRoutes();
    void startDnsIfNeeded();
};

extern WebConfigServer webConfigServer;

#endif // WEB_CONFIG_SERVER_H
