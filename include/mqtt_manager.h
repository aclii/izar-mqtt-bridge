#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include "config.h" // Include config first to get MQTT_MAX_PACKET_SIZE
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

typedef void (*MqttCallbackFunction)(const char* topic, const byte* payload, unsigned int length);

class MqttManager {
  private:
    WiFiClient espClient;
    PubSubClient client;
    unsigned long lastReconnectAttempt = 0;
    MqttCallbackFunction externalCallback = nullptr;
    bool discoveryPublished = false;

    char topicStatus[128]{};
    char topicReading[128]{};
    char topicCommand[128]{};

  public:
    MqttManager();

    bool init();
    bool connect();
    bool isConnected();
    void disconnect();
    void handle();

    // Publishing
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool publish(const char* topic, const JsonDocument& doc, bool retain = false);

    // Subscription
    bool subscribe(const char* topic);

    // Topic helpers
    const char* getTopicStatus() const;
    const char* getTopicReading() const;
    const char* getTopicCommand() const;

    // Callbacks
    void setCallback(MqttCallbackFunction callback);

  private:
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    static void staticMqttCallback(char* topic, byte* payload, unsigned int length);
    void updateTopics();
    bool publishDiscoveryEntity(const char* component, const char* objectId, const char* name, const char* stateTopic,
                                const char* unit, const char* deviceClass, const char* stateClass,
                                const char* valueTemplate, const char* icon);
    void publishDiscoveryAll();
};

extern MqttManager mqttManager;

#endif // MQTT_MANAGER_H
