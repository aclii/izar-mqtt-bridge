#include "mqtt_manager.h"
#include "wifi_manager.h"
#include "config_manager.h"
#include <ESPmDNS.h>
#include <WiFi.h>

MqttManager mqttManager;

MqttManager::MqttManager() : client(espClient) {}

bool MqttManager::init() {
    const Config& config = configManager.getConfig();
    client.setServer(config.mqttBroker, config.mqttPort);
    client.setCallback(staticMqttCallback);
    client.setBufferSize(MQTT_MAX_PACKET_SIZE);
    updateTopics();
    discoveryPublished = false;
    LOG_INFO("MQTT", "Manager initialized successfully");
    return true;
}

bool MqttManager::connect() {
    if (client.connected()) {
        return true;
    }

    if (!wifiManager.isWiFiConnected()) {
        LOG_DEBUG("MQTT", "WiFi not connected");
        return false;
    }

    const Config& config = configManager.getConfig();

    IPAddress resolvedIp;
    bool resolved = false;
    String broker = String(config.mqttBroker);
    if (broker.endsWith(".local")) {
        String host = broker.substring(0, broker.length() - 6);
        IPAddress mdnsIp = MDNS.queryHost(host);
        if (mdnsIp != INADDR_NONE) {
            resolvedIp = mdnsIp;
            resolved = true;
            LOG_INFO("MQTT", "mDNS resolved %s to %s", config.mqttBroker, resolvedIp.toString().c_str());
        }
    }

    if (!resolved) {
        resolved = WiFi.hostByName(config.mqttBroker, resolvedIp);
        if (resolved) {
            LOG_INFO("MQTT", "DNS resolved %s to %s", config.mqttBroker, resolvedIp.toString().c_str());
        }
    }

    if (resolved) {
        client.setServer(resolvedIp, config.mqttPort);
    } else {
        client.setServer(config.mqttBroker, config.mqttPort);
    }
    updateTopics();

    const char* clientId = strlen(config.mqttClientId) > 0 ? config.mqttClientId : MQTT_CLIENT_ID_DEFAULT;
    LOG_INFO("MQTT", "Connecting to %s:%d", config.mqttBroker, config.mqttPort);

    bool connected = false;
    if (strlen(config.mqttUsername) > 0) {
        connected =
            client.connect(clientId, config.mqttUsername, config.mqttPassword, getTopicStatus(), 1, true, "offline");
    } else {
        connected = client.connect(clientId, nullptr, nullptr, getTopicStatus(), 1, true, "offline");
    }

    if (connected) {
        LOG_INFO("MQTT", "Connected!");
        publish(getTopicStatus(), "online", true);
        subscribe(getTopicCommand());
        if (!discoveryPublished) {
            publishDiscoveryAll();
            discoveryPublished = true;
        }
        return true;
    } else {
        LOG_ERROR("MQTT", "Connection failed, rc=%d", client.state());
        return false;
    }
}

bool MqttManager::isConnected() {
    return client.connected();
}

void MqttManager::disconnect() {
    if (client.connected()) {
        client.disconnect();
        LOG_INFO("MQTT", "Disconnected");
    }
}

void MqttManager::handle() {
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            if (connect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        client.loop();
    }
}

bool MqttManager::publish(const char* topic, const char* payload, bool retain) {
    if (!client.connected()) {
        return false;
    }

    bool result = client.publish(topic, reinterpret_cast<const uint8_t*>(payload), strlen(payload), retain);
    if (result) {
        LOG_DEBUG("MQTT", "Published to %s: %s", topic, payload);
    } else {
        LOG_ERROR("MQTT", "Failed to publish to %s", topic);
    }
    return result;
}

bool MqttManager::publish(const char* topic, const JsonDocument& doc, bool retain) {
    String payload;
    serializeJson(doc, payload);
    return publish(topic, payload.c_str(), retain);
}

bool MqttManager::subscribe(const char* topic) {
    if (!client.connected()) {
        return false;
    }

    bool result = client.subscribe(topic);
    if (result) {
        LOG_INFO("MQTT", "Subscribed to %s", topic);
    }
    return result;
}

void MqttManager::setCallback(MqttCallbackFunction callback) {
    externalCallback = callback;
}

void MqttManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    LOG_DEBUG("MQTT", "Message from %s", topic);

    if (externalCallback != nullptr) {
        externalCallback(topic, payload, length);
    }
}

void MqttManager::staticMqttCallback(char* topic, byte* payload, unsigned int length) {
    mqttManager.mqttCallback(topic, payload, length);
}

void MqttManager::updateTopics() {
    const Config& config = configManager.getConfig();
    String base = strlen(config.mqttBaseTopic) > 0 ? String(config.mqttBaseTopic) : String(MQTT_BASE_TOPIC_DEFAULT);
    if (base.endsWith("/")) {
        base.remove(base.length() - 1);
    }

    snprintf(topicStatus, sizeof(topicStatus), "%s/status", base.c_str());
    snprintf(topicReading, sizeof(topicReading), "%s/reading", base.c_str());
    snprintf(topicCommand, sizeof(topicCommand), "%s/cmd", base.c_str());
}

bool MqttManager::publishDiscoveryEntity(const char* component, const char* objectId, const char* name,
                                         const char* stateTopic, const char* unit, const char* deviceClass,
                                         const char* stateClass, const char* valueTemplate, const char* icon) {
    const Config& config = configManager.getConfig();
    const char* deviceId = strlen(config.mqttClientId) > 0 ? config.mqttClientId : HA_DEVICE_ID;

    char discoveryTopic[256];
    snprintf(discoveryTopic, sizeof(discoveryTopic), "%s/%s/%s/%s/config", HA_DISCOVERY_PREFIX, component, deviceId,
             objectId);

    DynamicJsonDocument doc(768);
    doc["name"] = name;
    doc["unique_id"] = String(deviceId) + "_" + objectId;
    doc["state_topic"] = stateTopic;
    if (unit && strlen(unit) > 0) {
        doc["unit_of_measurement"] = unit;
    }
    if (deviceClass && strlen(deviceClass) > 0) {
        doc["device_class"] = deviceClass;
    }
    if (stateClass && strlen(stateClass) > 0) {
        doc["state_class"] = stateClass;
    }
    if (valueTemplate && strlen(valueTemplate) > 0) {
        doc["value_template"] = valueTemplate;
    }
    if (icon && strlen(icon) > 0) {
        doc["icon"] = icon;
    }

    if (strcmp(objectId, "current_reading") == 0 || strcmp(objectId, "h0_reading") == 0) {
        doc["suggested_display_precision"] = 3;
    }

    doc["availability_topic"] = getTopicStatus();
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";

    if (strcmp(component, "binary_sensor") == 0) {
        doc["payload_on"] = "true";
        doc["payload_off"] = "false";
    }

    JsonObject device = doc["device"].to<JsonObject>();
    device["name"] = HA_DEVICE_NAME;
    device["identifiers"][0] = deviceId;
    device["manufacturer"] = "aclii";
    device["model"] = PROJECT_NAME " (M5Stack Unit-C6L)";
    device["sw_version"] = PROJECT_VERSION;
    String configUrl = String("http://") + WiFi.localIP().toString() + "/";
    device["configuration_url"] = configUrl;
    JsonArray connections = device.createNestedArray("connections");
    JsonArray conn = connections.createNestedArray();
    conn.add("mac");
    conn.add(WiFi.macAddress());

    return publish(discoveryTopic, doc, true);
}

void MqttManager::publishDiscoveryAll() {
    publishDiscoveryEntity("sensor", "meter_id", "Meter ID", getTopicReading(), nullptr, nullptr, nullptr,
                           "{{ value_json.meter_id }}", "mdi:identifier");

    publishDiscoveryEntity("sensor", "flow_rate", "Flow Rate", getTopicReading(), "l/h", nullptr, "measurement",
                           "{{ '%.2f'|format(value_json.flow_rate) }}", "mdi:water");

    publishDiscoveryEntity("sensor", "current_reading", "Current Reading", getTopicReading(), "m³", "water",
                           "total_increasing", "{{ '%.3f'|format(value_json.current_reading) }}", "mdi:water");

    publishDiscoveryEntity("sensor", "h0_reading", "Checkpoint Reading", getTopicReading(), "m³", "water", nullptr,
                           "{{ '%.3f'|format(value_json.h0_reading) }}", "mdi:water-check");

    publishDiscoveryEntity("sensor", "h0_date", "Checkpoint Date", getTopicReading(), nullptr, nullptr, nullptr,
                           "{{ value_json.h0_date }}", "mdi:calendar");

    publishDiscoveryEntity("sensor", "battery_years", "Battery Remaining", getTopicReading(), "years", nullptr,
                           "measurement", "{{ value_json.battery_years }}", "mdi:battery-clock");

    publishDiscoveryEntity("sensor", "radio_interval", "Radio Interval", getTopicReading(), "s", "duration", nullptr,
                           "{{ value_json.radio_interval }}", "mdi:timer");

    publishDiscoveryEntity("sensor", "meter_rssi", "Meter RSSI", getTopicReading(), "dBm", "signal_strength", nullptr,
                           "{{ value_json.meter_rssi }}", "mdi:wifi");

    publishDiscoveryEntity("sensor", "wifi_rssi", "WiFi RSSI", getTopicReading(), "dBm", "signal_strength", nullptr,
                           "{{ value_json.wifi_rssi }}", "mdi:wifi");

    publishDiscoveryEntity("sensor", "free_heap_kb", "Free Heap", getTopicReading(), "kB", "data_size", "measurement",
                           "{{ '%.1f'|format(value_json.free_heap_kb) }}", "mdi:memory");

    publishDiscoveryEntity(
        "binary_sensor", "alarm_any", "Alarm - Any", getTopicReading(), nullptr, "problem", nullptr,
        "{{ 'true' if (value_json.alarms.general or value_json.alarms.leakage_current or "
        "value_json.alarms.meter_blocked or value_json.alarms.back_flow or value_json.alarms.underflow or "
        "value_json.alarms.overflow or value_json.alarms.submarine or value_json.alarms.sensor_fraud_current or "
        "value_json.alarms.mechanical_fraud_current) else 'false' }}",
        "mdi:alert");

    publishDiscoveryEntity("binary_sensor", "alarm_general", "Alarm - General", getTopicReading(), nullptr, "problem",
                           nullptr, "{{ 'true' if value_json.alarms.general else 'false' }}", "mdi:alert-circle");

    publishDiscoveryEntity("binary_sensor", "alarm_leakage_current", "Alarm - Leakage (Current)", getTopicReading(),
                           nullptr, "problem", nullptr,
                           "{{ 'true' if value_json.alarms.leakage_current else 'false' }}", "mdi:water-alert");

    publishDiscoveryEntity("binary_sensor", "alarm_leakage_previous", "Alarm - Leakage (Previous)", getTopicReading(),
                           nullptr, "problem", nullptr,
                           "{{ 'true' if value_json.alarms.leakage_previous else 'false' }}", "mdi:water-alert");

    publishDiscoveryEntity("binary_sensor", "alarm_meter_blocked", "Alarm - Meter Blocked", getTopicReading(), nullptr,
                           "problem", nullptr, "{{ 'true' if value_json.alarms.meter_blocked else 'false' }}",
                           "mdi:alert-octagon");

    publishDiscoveryEntity("binary_sensor", "alarm_back_flow", "Alarm - Backflow", getTopicReading(), nullptr,
                           "problem", nullptr, "{{ 'true' if value_json.alarms.back_flow else 'false' }}",
                           "mdi:backup-restore");

    publishDiscoveryEntity("binary_sensor", "alarm_underflow", "Alarm - Underflow", getTopicReading(), nullptr,
                           "problem", nullptr, "{{ 'true' if value_json.alarms.underflow else 'false' }}",
                           "mdi:water-minus");

    publishDiscoveryEntity("binary_sensor", "alarm_overflow", "Alarm - Overflow", getTopicReading(), nullptr, "problem",
                           nullptr, "{{ 'true' if value_json.alarms.overflow else 'false' }}", "mdi:water-plus");

    publishDiscoveryEntity("binary_sensor", "alarm_submarine", "Alarm - Submarine", getTopicReading(), nullptr,
                           "problem", nullptr, "{{ 'true' if value_json.alarms.submarine else 'false' }}",
                           "mdi:submarine");

    publishDiscoveryEntity("binary_sensor", "alarm_sensor_fraud_current", "Alarm - Sensor Fraud (Current)",
                           getTopicReading(), nullptr, "problem", nullptr,
                           "{{ 'true' if value_json.alarms.sensor_fraud_current else 'false' }}", "mdi:alert-decagram");

    publishDiscoveryEntity("binary_sensor", "alarm_sensor_fraud_previous", "Alarm - Sensor Fraud (Previous)",
                           getTopicReading(), nullptr, "problem", nullptr,
                           "{{ 'true' if value_json.alarms.sensor_fraud_previous else 'false' }}",
                           "mdi:alert-decagram");

    publishDiscoveryEntity("binary_sensor", "alarm_mechanical_fraud_current", "Alarm - Mechanical Fraud (Current)",
                           getTopicReading(), nullptr, "problem", nullptr,
                           "{{ 'true' if value_json.alarms.mechanical_fraud_current else 'false' }}",
                           "mdi:alert-decagram");

    publishDiscoveryEntity("binary_sensor", "alarm_mechanical_fraud_previous", "Alarm - Mechanical Fraud (Previous)",
                           getTopicReading(), nullptr, "problem", nullptr,
                           "{{ 'true' if value_json.alarms.mechanical_fraud_previous else 'false' }}",
                           "mdi:alert-decagram");
}

const char* MqttManager::getTopicStatus() const {
    return topicStatus;
}
const char* MqttManager::getTopicReading() const {
    return topicReading;
}
const char* MqttManager::getTopicCommand() const {
    return topicCommand;
}
