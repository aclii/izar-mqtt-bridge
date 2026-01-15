#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "web_logger.h"

#define PROJECT_NAME "IZAR RC 686 I W R4 Water Meter MQTT Bridge"
#define PROJECT_VERSION "1.0.0"

// ============ WiFi Configuration Defaults ============
#define WIFI_SSID_DEFAULT "YOUR_SSID"
#define WIFI_PASSWORD_DEFAULT "YOUR_PASSWORD"
#define WIFI_HOSTNAME_DEFAULT "izar-mqtt-bridge"
#define WIFI_CONNECTION_TIMEOUT 20000 // ms

// ============ WiFi Access Point Defaults ============
#define AP_SSID_PREFIX_DEFAULT "izar-mqtt-bridge-config"
#define AP_PASSWORD_DEFAULT "izar-mqtt"

// ============ MQTT Configuration Defaults ============
#define MQTT_BROKER_DEFAULT "homeassistant.local" // local MQTT broker IP or hostname (e.g. homeassistant.local)
#define MQTT_PORT_DEFAULT 1883
#define MQTT_CLIENT_ID_DEFAULT "izar_water_meter_bridge"
#define MQTT_USERNAME_DEFAULT ""
#define MQTT_PASSWORD_DEFAULT ""
#define MQTT_BASE_TOPIC_DEFAULT "home/water_meter"

// ============ IZAR Defaults ============
#define IZAR_SERIAL_NUMBER_DEFAULT ""

// Home Assistant Discovery
#define HA_DISCOVERY_PREFIX "homeassistant"
#define HA_DEVICE_NAME "IZAR Water Meter"
#define HA_DEVICE_ID "izar_water_meter"

// ============ Hardware Configuration ============
// Device: Stamp C6 with ESP32-C6 + SX1262 FSK Modem + SSD1306 + PI4IOE5V6408 GPIO Expander

// SX1262 SSD1306 shared SPI Configuration ==========
#define SX1262_SSD1306_MOSI_PIN 21 // SPI MOSI
#define SX1262_SSD1306_MISO_PIN 22 // SPI MISO
#define SX1262_SSD1306_SCK_PIN 20  // SPI Clock

// ========== SX1262 FSK Modem IO Configuration ==========
#define SX1262_CS_PIN 23   // SPI Chip Select
#define SX1262_BUSY_PIN 19 // Busy Signal
#define SX1262_IRQ_PIN 7   // Interrupt Pin

// ========== SSD1306 OLED IO Configuration ==========
#define SSD1306_CS_PIN 6   // SPI Chip Select
#define SSD1306_DC_PIN 18  // Data/Command Pin
#define SSD1306_RST_PIN 15 // Reset Pin

// ========== I2C GPIO Expander (PI4IOE5V6408) ==========
// This I2C expander handles User Button, SX1262 control pins, and more
#define GPIO_EXPANDER_SDA_PIN 10       // I2C SDA
#define GPIO_EXPANDER_SCL_PIN 8        // I2C SCL
#define GPIO_EXPANDER_INT_PIN 3        // Interrupt output
#define GPIO_EXPANDER_RST_PIN -1       // RST (connected to ESP_RST - no GPIO pin needed)
#define GPIO_EXPANDER_I2C_ADDR 0x43    // I2C address (Unit-C6L specific)
#define GPIO_EXPANDER_I2C_CLOCK 100000 // I2C clock (100kHz)

// GPIO Expander Output Pins (E0.Px format)
#define GPIO_EXPANDER_IO_SYS_KEY1 0  // E0.P0 = SYS_KEY1 (User button)
#define GPIO_EXPANDER_IO_SX_LNA_EN 5 // E0.P5 = SX_LNA_EN (SX1262 RF amplifier enable)
#define GPIO_EXPANDER_IO_SX_ANT_SW 6 // E0.P6 = SX_ANT_SW (SX1262 RF switch)
#define GPIO_EXPANDER_IO_SX_NRST 7   // E0.P7 = SX_NRST (SX1262 reset)

// ========== RGB LED & Buzzer ==========
#define RGB_LED_PIN 2 // WS2812C addressable RGB (single LED)
#define BUZZER_PIN 11 // Buzzer

// ============ Display Configuration ============
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 48
#define DISPLAY_BRIGHTNESS 32        // 0-255
#define DISPLAY_UPDATE_INTERVAL 1000 // ms
#define DISPLAY_TIMEOUT 30000        // ms - turn off display after 30 seconds of inactivity

// ============ SPI Configuration ============
#define SX1262_SSD1306_SPI_FREQUENCY 8000000UL // 8 MHz SPI for display and FSK modem

// ============ FSK Modem Configuration ============
#define FSK_MODEM_FREQUENCY 868.95F         // 868.95 MHz - wM-Bus T1 mode default frequency
#define FSK_MODEM_BIT_RATE 100.0F           // 100 kbps - wM-Bus T1 mode default bit rate
#define FSK_MODEM_FREQUENCY_DEVIATION 50.0F // 50 kHz - wM-Bus T1 mode default frequency deviation
#define FSK_MODEM_RX_BANDWIDTH 234.3F       // 234.3 kHz - Closest available bandwidth to 200 kHz
// clang-format off
#define FSK_MODEM_SYNC_WORD                                                                                            \
    {0x55, 0x55, 0x54, 0x3D}       // RadioLib default preamble is 16 bits for FSK
                                   // wM-Bus T1 mode uses 38 bits preamble + 10 bits sync word
                                   // Padding with 0x55 to align to preamble bytes
// clang-format on
#define FSK_MODEM_RX_MAX_LENGTH 64 // Max payload length bytes, IZAR uses 38 bytes (3-out-of-6 encoding) in R3 mode

// ============ Features ============
#define ENABLE_DISPLAY true
#define ENABLE_FSK_MODEM true // Wireless meter reading via FSK modem
#define ENABLE_BUZZER true
#define ENABLE_RGB_LED true

// ============ Timing Configuration ============
#define MQTT_RECONNECT_INTERVAL 5000 // ms
#define MQTT_PUBLISH_INTERVAL 300000 // ms - publish every 5 minutes

// ============ Memory & Performance ============
#define MQTT_MAX_PACKET_SIZE 1024
#define JSON_BUFFER_SIZE 512

// ============ Logging Configuration ============
// Log levels: 0=NONE, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

// Set global log level (change this to control verbosity)
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Logging macros
#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(tag, ...)                                                                                            \
    do {                                                                                                               \
        Serial.printf("[ERROR][%s] ", tag);                                                                            \
        Serial.printf(__VA_ARGS__);                                                                                    \
        Serial.println();                                                                                              \
        webLogger.logf(LOG_LEVEL_ERROR, tag, __VA_ARGS__);                                                             \
    } while (0)
#else
#define LOG_ERROR(tag, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
#define LOG_WARN(tag, ...)                                                                                             \
    do {                                                                                                               \
        Serial.printf("[WARN][%s] ", tag);                                                                             \
        Serial.printf(__VA_ARGS__);                                                                                    \
        Serial.println();                                                                                              \
        webLogger.logf(LOG_LEVEL_WARNING, tag, __VA_ARGS__);                                                           \
    } while (0)
#else
#define LOG_WARN(tag, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(tag, ...)                                                                                             \
    do {                                                                                                               \
        Serial.printf("[INFO][%s] ", tag);                                                                             \
        Serial.printf(__VA_ARGS__);                                                                                    \
        Serial.println();                                                                                              \
        webLogger.logf(LOG_LEVEL_INFO, tag, __VA_ARGS__);                                                              \
    } while (0)
#else
#define LOG_INFO(tag, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(tag, ...)                                                                                            \
    do {                                                                                                               \
        Serial.printf("[DEBUG][%s] ", tag);                                                                            \
        Serial.printf(__VA_ARGS__);                                                                                    \
        Serial.println();                                                                                              \
        webLogger.logf(LOG_LEVEL_DEBUG, tag, __VA_ARGS__);                                                             \
    } while (0)
#else
#define LOG_DEBUG(tag, ...)
#endif

// Always print (bypass log level)
#define LOG_ALWAYS(...)                                                                                                \
    Serial.printf(__VA_ARGS__);                                                                                        \
    Serial.println()

#endif // CONFIG_H
