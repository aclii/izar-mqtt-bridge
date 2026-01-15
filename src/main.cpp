#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "config_manager.h"
#include "gpio_expander_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "display_manager.h"
#include "fsk_modem_manager.h"
#include "wm_bus_handler.h"
#include "prios_handler.h"
#include "izar_handler.h"
#include "hardware_manager.h"
#include "web_config_server.h"

// Timing variables
unsigned long lastStatusPublish = 0;

// Meter binding state
enum MeterBindingState {
    METER_BINDING_STATE_DISCOVERY, // Discovering meters
    METER_BINDING_STATE_BOUND      // Meter selected and bound
};

MeterBindingState bindingState = METER_BINDING_STATE_DISCOVERY;
std::vector<String> discoveredMeters; // List of unique meter IDs
std::vector<int16_t> discoveredRSSI;  // RSSI values for each meter
int selectedMeterIndex = 0;           // Currently selected meter in list
String boundMeterId = "";             // The meter we're bound to
IzarReading latestReading;            // Latest reading from bound meter
bool hasReading = false;              // Whether we have received a reading yet
unsigned long lastUpdateTime = 0;     // Time of last meter update (millis)
unsigned long lastActivityTime = 0;   // Time of last user activity (millis)
bool displayAsleep = false;           // Display sleep state

// Forward declarations
void mqttMessageCallback(const char* topic, const byte* payload, unsigned int length);
void fskModemMessageCallback(const uint8_t* payload, uint8_t length, int rssi);
void wmBusPacketCallback(const char* meterId, const uint8_t* fullwMBusFrame, uint8_t fullwMBusFrameLen,
                         uint8_t* tplData, uint8_t tplDataLen, int16_t rssi);
void izarDataCallback(const IzarReading* reading);
void updateDisplay();
void handleButtonPress();

void updateDisplay() {
    if (!ENABLE_DISPLAY)
        return;

    // Clear screen and prepare for aligned printing
    displayManager.clear();

    if (bindingState == METER_BINDING_STATE_DISCOVERY) {
        if (discoveredMeters.empty()) {
            // Scanning state
            String display = "Scanning\nIZAR\nmeters";
            displayManager.printAligned(display.c_str(), DisplayManager::HAlign::CENTER,
                                        DisplayManager::VAlign::MIDDLE);
        } else {
            // Build multi-line display
            String display = "Found IZAR\n";

            // Meter ID - truncate to 10 chars if needed
            String meterId = discoveredMeters[selectedMeterIndex];
            if (meterId.length() > 10) {
                meterId = meterId.substring(meterId.length() - 10);
            }
            display += meterId + "\n";

            // RSSI
            String rssiStr = String(discoveredRSSI[selectedMeterIndex]) + " dBm";
            display += rssiStr + "\n";

            // Instructions
            display += "Hold->bind\n";

            // Navigation with brackets
            String pageNum = String(selectedMeterIndex + 1) + "/" + String(discoveredMeters.size());
            String nav = "";
            if (selectedMeterIndex == 0) {
                nav += "[";
            } else {
                nav += "<";
            }
            nav += "   " + pageNum + "   ";
            if (selectedMeterIndex == (int)discoveredMeters.size() - 1) {
                nav += "]";
            } else {
                nav += ">";
            }
            display += nav;

            displayManager.printAligned(display.c_str(), DisplayManager::HAlign::CENTER,
                                        DisplayManager::VAlign::MIDDLE);
        }
    } else {
        // Bound mode - show meter status
        if (!hasReading) {
            // Waiting for data
            String display = "Awaiting\ndata from\n" +
                             boundMeterId.substring(boundMeterId.length() > 10 ? boundMeterId.length() - 10 : 0);
            displayManager.printAligned(display.c_str(), DisplayManager::HAlign::CENTER,
                                        DisplayManager::VAlign::MIDDLE);
        } else {
            // Build status display with full meter info
            String display = "";

            // Line 1: Full meter ID (truncate if longer than 10 chars)
            String meterId = String(latestReading.meterId);
            if (meterId.length() > 10) {
                meterId = meterId.substring(meterId.length() - 10);
            }
            display += meterId + "\n";

            // Line 2: Current reading
            char readingBuf[16];
            snprintf(readingBuf, sizeof(readingBuf), "%.3f m3", latestReading.current_reading);
            display += String(readingBuf) + "\n";

            // Line 3: RSSI
            display += String(latestReading.rssi) + " dBm\n";

            // Line 4: Battery life
            char batteryBuf[16];
            snprintf(batteryBuf, sizeof(batteryBuf), "%.1f years", latestReading.remaining_battery_life);
            display += String(batteryBuf) + "\n";

            // Line 5: Alarms with lowercase/uppercase coding
            // [GLBRUSFM]: General, Leakage, Blocked, backflow(R), Underflow, Submarine, sensor Fraud, Mechanical fraud
            // Lowercase = no alarm, Uppercase = alarm active
            display += "[";

            // G - general alarm
            display += latestReading.alarms.general_alarm ? 'G' : 'g';

            // L - leakage alarm
            display += latestReading.alarms.leakage_currently ? 'L' : 'l';

            // B - blocked meter
            display += latestReading.alarms.meter_blocked ? 'B' : 'b';

            // R - backflow alarm
            display += latestReading.alarms.back_flow ? 'R' : 'r';

            // U - underflow alarm
            display += latestReading.alarms.underflow ? 'U' : 'u';

            // S - submarine alarm
            display += latestReading.alarms.submarine ? 'S' : 's';

            // F - sensor fraud
            display += latestReading.alarms.sensor_fraud_currently ? 'F' : 'f';

            // M - mechanical fraud
            display += latestReading.alarms.mechanical_fraud_currently ? 'M' : 'm';

            display += "]";

            displayManager.printAligned(display.c_str(), DisplayManager::HAlign::CENTER,
                                        DisplayManager::VAlign::MIDDLE);

            // Draw timeout indicator (vertical line on left side)
            // Max timeout is 200% of radio_interval
            unsigned long elapsedSeconds = (millis() - lastUpdateTime) / 1000;
            int maxTimeout = latestReading.radio_interval * 2; // 200% of update interval

            // Calculate line height (48 pixels for display height)
            int lineHeight = 0;
            if (elapsedSeconds <= maxTimeout && maxTimeout > 0) {
                lineHeight = (elapsedSeconds * 48) / maxTimeout;
                if (lineHeight > 48)
                    lineHeight = 48;
            } else {
                // Timeout exceeded - draw full line
                lineHeight = 48;
            }

            // Draw vertical line from bottom to top on left edge (x=0)
            displayManager.drawTimeoutLine(lineHeight);
        }
    }
}

void handleButtonPress() {
    ButtonEvent event = hardwareManager.getButtonEvent();

    // Wake display on any button activity
    if (displayAsleep && event != ButtonEvent::BUTTON_EVENT_NONE) {
        displayManager.wake();
        displayAsleep = false;
        if (ENABLE_DISPLAY) {
            updateDisplay();
        }
    }

    // Reset activity timer on button press
    if (event != ButtonEvent::BUTTON_EVENT_NONE) {
        lastActivityTime = millis();
    }

    if (event == ButtonEvent::BUTTON_EVENT_NONE || bindingState != METER_BINDING_STATE_DISCOVERY ||
        discoveredMeters.empty()) {
        return;
    }

    if (event == ButtonEvent::BUTTON_EVENT_LONG_PRESS) {
        // Bind to selected meter
        boundMeterId = discoveredMeters[selectedMeterIndex];
        bindingState = METER_BINDING_STATE_BOUND;
        Config& config = configManager.getConfig();
        strlcpy(config.serialNumber, boundMeterId.c_str(), sizeof(config.serialNumber));
        configManager.save();
        LOG_INFO("Main", "Bound to meter: %s", boundMeterId.c_str());
        hardwareManager.beepSuccess();
        updateDisplay();
    } else if (event == ButtonEvent::BUTTON_EVENT_SHORT_PRESS) {
        // Scroll to next meter
        selectedMeterIndex = (selectedMeterIndex + 1) % discoveredMeters.size();
        LOG_DEBUG("Main", "Selected meter index: %d", selectedMeterIndex);
        hardwareManager.beep(50, 2000); // Short high beep
        updateDisplay();
    }
}

// cppcheck-suppress unusedFunction
void setup() {
// ESP32-C6 USB CDC initialization
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0); // Prevent blocking on Serial
#endif

    Serial.begin(115200);

    // Wait for USB CDC connection (timeout after 3 seconds)
    unsigned long start = millis();
    while (!Serial && (millis() - start < 3000)) {
        delay(10);
    }

    LOG_INFO("Main", "=== M5 Stack Water Meter (Unit-C6L) ===");
    LOG_INFO("Main", "Starting initialization...");

    // Load configuration from flash
    configManager.begin();

    // Apply binding state from config
    const Config& config = configManager.getConfig();
    if (strlen(config.serialNumber) > 0) {
        boundMeterId = config.serialNumber;
        bindingState = METER_BINDING_STATE_BOUND;
        LOG_INFO("Main", "Configured meter: %s (bound mode)", boundMeterId.c_str());
    } else {
        bindingState = METER_BINDING_STATE_DISCOVERY;
        boundMeterId = "";
        LOG_INFO("Main", "No configured meter (discovery mode)");
    }

    // Initialize hardware (including shared SPI bus)
    hardwareManager.init();
    delay(100);

    // Initialize display with shared SPI
    if (ENABLE_DISPLAY) {
        displayManager.init(hardwareManager.getSharedSPI());
    }

    // Initialize GPIO expander AFTER display (M5GFX interferes with I2C, so init I2C fresh here)
    gpioExpanderManager.init();
    delay(100);

    // Initialize wM-Bus handler
    wmBusHandler.init();
    wmBusHandler.setPacketCallback(wmBusPacketCallback);

    // Initialize PRIOS handler
    priosHandler.init();

    // Initialize IZAR handler
    izarHandler.init();
    izarHandler.setDataCallback(izarDataCallback);

    // Initialize wireless receiver (FSK modem for meter reading)
    if (ENABLE_FSK_MODEM) {
        fskModemManager.init(hardwareManager.getSharedSPI());
        fskModemManager.setCallback(fskModemMessageCallback);
    }

    // Initialize WiFi and MQTT
    wifiManager.init();
    wifiManager.connect();

    mqttManager.init();
    mqttManager.setCallback(mqttMessageCallback);
    mqttManager.connect();

    // Start web configuration server
    webConfigServer.begin();

    LOG_INFO("Main", "Initialization complete");

    // Initialize activity timer
    lastActivityTime = millis();

    // Initialize binding display
    if (ENABLE_DISPLAY) {
        updateDisplay();
    }
}

void loop() {
    unsigned long now = millis();

    // Keep WiFi and MQTT connected
    wifiManager.handleWiFi();
    mqttManager.handle();
    webConfigServer.handle();

    // FSK modem wireless meter reading
    if (ENABLE_FSK_MODEM) {
        fskModemManager.handle();
    }

    // Update display every second to refresh timeout indicator
    if (ENABLE_DISPLAY && bindingState == METER_BINDING_STATE_BOUND && hasReading) {
        static unsigned long lastDisplayUpdate = 0;
        if (now - lastDisplayUpdate >= 1000) {
            lastDisplayUpdate = now;
            if (!displayAsleep) {
                updateDisplay();
            }
        }
    }

    // Check display timeout
    if (ENABLE_DISPLAY && !displayAsleep && (now - lastActivityTime > DISPLAY_TIMEOUT)) {
        displayManager.sleep();
        displayAsleep = true;
        LOG_DEBUG("Main", "Display sleep due to inactivity %d seconds.", (now - lastActivityTime) / 1000);
    }

    // Button handling for meter binding
    handleButtonPress();

    delay(10); // Small delay to prevent watchdog triggers
}

void mqttMessageCallback(const char* topic, const byte* payload, unsigned int length) {
    LOG_INFO("Main", "MQTT message on topic: %s", topic);

    // Create null-terminated string from payload
    char message[256];
    if (length < sizeof(message)) {
        strncpy(message, reinterpret_cast<const char*>(payload), length);
        message[length] = '\0';
    }

    // Handle commands
    if (strcmp(topic, mqttManager.getTopicCommand()) == 0) {
        if (strcmp(message, "beep") == 0) {
            hardwareManager.beepSuccess();
        } else if (strcmp(message, "reset") == 0) {
            LOG_INFO("Main", "Reset requested");
            ESP.restart();
        } else if (strcmp(message, "status") == 0) {
            mqttManager.publish(mqttManager.getTopicStatus(), "online", true);
        }
    }
}

void fskModemMessageCallback(const uint8_t* payload, uint8_t length, int rssi) {
    LOG_DEBUG("Main", "FSK modem packet received: %d bytes, RSSI=%d dBm", length, rssi);

    // Pass raw packet to wM-Bus handler for decoding and parsing
    if (length > 0) {
        wmBusHandler.processRawPacket(payload, length, rssi);
    } else {
        LOG_DEBUG("Main", "Received empty packet");
    }
}

// Callback for successfully parsed wM-Bus packets
void wmBusPacketCallback(const char* meterId, const uint8_t* fullwMBusFrame, uint8_t fullwMBusFrameLen,
                         uint8_t* tplData, uint8_t tplDataLen, int16_t rssi) {
    LOG_INFO("Main", "wM-Bus packet parsed: meter=%s, wM-BusFrame=%d bytes, tplData=%d bytes, RSSI=%d dBm", meterId,
             fullwMBusFrameLen, tplDataLen, rssi);

    if (bindingState == METER_BINDING_STATE_DISCOVERY) {
        const Config& config = configManager.getConfig();
        if (strlen(config.serialNumber) > 0 && String(meterId) == String(config.serialNumber)) {
            boundMeterId = meterId;
            bindingState = METER_BINDING_STATE_BOUND;
            LOG_INFO("Main", "Auto-bound to configured meter: %s", boundMeterId.c_str());
            hardwareManager.beepSuccess();
            if (ENABLE_DISPLAY) {
                updateDisplay();
            }
            return;
        }

        // Discovery mode - add to list if new or update RSSI
        String meterIdStr = String(meterId);
        int existingIndex = -1;

        for (int i = 0; i < discoveredMeters.size(); i++) {
            if (discoveredMeters[i] == meterIdStr) {
                existingIndex = i;
                break;
            }
        }

        if (existingIndex == -1) {
            // New meter
            discoveredMeters.push_back(meterIdStr);
            discoveredRSSI.push_back(rssi);
            LOG_INFO("Main", "New meter discovered: %s (total: %d)", meterId, discoveredMeters.size());
            if (ENABLE_DISPLAY) {
                updateDisplay();
            }
        } else {
            // Update RSSI for existing meter
            discoveredRSSI[existingIndex] = rssi;
            // Update display if this is the currently selected meter
            if (existingIndex == selectedMeterIndex && ENABLE_DISPLAY) {
                updateDisplay();
            }
        }

        // Don't process packets in discovery mode
        return;
    }

    // Bound mode - filter by bound meter ID
    if (bindingState == METER_BINDING_STATE_BOUND && String(meterId) != boundMeterId) {
        LOG_DEBUG("Main", "Ignoring packet from non-bound meter: %s", meterId);
        return;
    }

    // Pass to PRIOS handler with full frame for proper LFSR initialization
    if (tplDataLen > 0 && fullwMBusFrameLen >= 12) {
        LOG_DEBUG("Main", "Passing to PRIOS handler...");
        priosHandler.processPayload(meterId, fullwMBusFrame, fullwMBusFrameLen, tplData, tplDataLen, rssi);
    } else {
        LOG_DEBUG("Main", "Insufficient data for PRIOS decryption");
    }
}

// Callback for decoded IZAR meter data
void izarDataCallback(const IzarReading* reading) {
    LOG_INFO("Main", "Water meter reading: ID=%s, Current=%.3f m³, H0=%.3f m³, RSSI=%d dBm", reading->meterId,
             reading->current_reading, reading->h0_reading, reading->rssi);

    // Store latest reading for display
    if (bindingState == METER_BINDING_STATE_BOUND) {
        memcpy(&latestReading, reading, sizeof(IzarReading));
        hasReading = true;
        lastUpdateTime = millis(); // Reset timeout timer

        // Wake display if asleep
        if (displayAsleep) {
            displayManager.wake();
            displayAsleep = false;
        }

        // Update display
        if (ENABLE_DISPLAY) {
            updateDisplay();
        }
    }

    // Publish to MQTT if connected
    if (mqttManager.isConnected()) {
        constexpr size_t kFlowWindowSize = 9;
        static float readingWindow[kFlowWindowSize]{};
        static unsigned long timeWindow[kFlowWindowSize]{};
        static size_t windowCount = 0;
        static size_t windowHead = 0;

        unsigned long now = millis();
        readingWindow[windowHead] = reading->current_reading;
        timeWindow[windowHead] = now;
        windowHead = (windowHead + 1) % kFlowWindowSize;
        if (windowCount < kFlowWindowSize) {
            windowCount++;
        }

        float flowLph = 0.0f;
        if (windowCount >= 2) {
            size_t oldestIndex = (windowHead + kFlowWindowSize - windowCount) % kFlowWindowSize;
            size_t newestIndex = (windowHead + kFlowWindowSize - 1) % kFlowWindowSize;

            float deltaM3 = readingWindow[newestIndex] - readingWindow[oldestIndex];
            float deltaLiters = deltaM3 * 1000.0f;
            unsigned long deltaMs = timeWindow[newestIndex] - timeWindow[oldestIndex];

            if (deltaMs > 0 && deltaLiters >= 1.0f) {
                flowLph = (deltaLiters * 3600000.0f) / static_cast<float>(deltaMs);
            }
        }

        StaticJsonDocument<768> doc;
        doc["meter_id"] = reading->meterId;
        doc["current_reading"] = reading->current_reading;
        doc["h0_reading"] = reading->h0_reading;
        doc["unit"] = reading->unit_type == VOLUME_CUBIC_METER ? "m3" : "unknown";
        doc["battery_years"] = reading->remaining_battery_life;
        doc["radio_interval"] = reading->radio_interval;
        doc["meter_rssi"] = reading->rssi;
        doc["wifi_rssi"] = wifiManager.getRSSI();
        doc["flow_rate"] = flowLph;
        doc["free_heap_kb"] = ESP.getFreeHeap() / 1024.0f;

        char h0Date[16];
        snprintf(h0Date, sizeof(h0Date), "%04u-%02u-%02u", reading->h0_year, reading->h0_month, reading->h0_day);
        doc["h0_date"] = h0Date;

        JsonObject alarms = doc.createNestedObject("alarms");
        alarms["general"] = reading->alarms.general_alarm;
        alarms["leakage_current"] = reading->alarms.leakage_currently;
        alarms["leakage_previous"] = reading->alarms.leakage_previously;
        alarms["meter_blocked"] = reading->alarms.meter_blocked;
        alarms["back_flow"] = reading->alarms.back_flow;
        alarms["underflow"] = reading->alarms.underflow;
        alarms["overflow"] = reading->alarms.overflow;
        alarms["submarine"] = reading->alarms.submarine;
        alarms["sensor_fraud_current"] = reading->alarms.sensor_fraud_currently;
        alarms["sensor_fraud_previous"] = reading->alarms.sensor_fraud_previously;
        alarms["mechanical_fraud_current"] = reading->alarms.mechanical_fraud_currently;
        alarms["mechanical_fraud_previous"] = reading->alarms.mechanical_fraud_previously;

        mqttManager.publish(mqttManager.getTopicReading(), doc);
    }
}
