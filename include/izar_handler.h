#ifndef IZAR_HANDLER_H
#define IZAR_HANDLER_H

#include <Arduino.h>
#include "config.h"

// IZAR data offsets (after CI byte 0x4B)
#define IZAR_OFFSET_STATUS_0 1        // Radio interval, random generator, general alarm
#define IZAR_OFFSET_STATUS_1 2        // Battery life, leakage alarms, meter blocked
#define IZAR_OFFSET_STATUS_2 3        // Backflow, overflow, fraud alarms
#define IZAR_OFFSET_UNITS 4           // Unit type and multiplier exponent
#define IZAR_OFFSET_MAGIC_BYTE 5      // Magic byte (0x4B)
#define IZAR_OFFSET_CURRENT_READING 6 // Current reading (4 bytes, little-endian)
#define IZAR_OFFSET_H0_READING 10     // H0 reading (4 bytes, little-endian)
#define IZAR_OFFSET_H0_DATE_DAY 14    // H0 date: day (5 bits) + year high (3 bits)
#define IZAR_OFFSET_H0_DATE_MONTH 15  // H0 date: month (4 bits) + year low (4 bits)
#define IZAR_MIN_DATA_LENGTH 16       // Minimum data length for IZAR reading

// IZAR unit types
enum IzarUnitType { UNKNOWN_UNIT = 0, VOLUME_CUBIC_METER = 2 };

// IZAR alarm flags
struct IzarAlarms {
    bool general_alarm;
    bool leakage_currently;
    bool leakage_previously;
    bool meter_blocked;
    bool back_flow;
    bool underflow;
    bool overflow;
    bool submarine;
    bool sensor_fraud_currently;
    bool sensor_fraud_previously;
    bool mechanical_fraud_currently;
    bool mechanical_fraud_previously;
};

// IZAR meter reading structure
struct IzarReading {
    char meterId[12]; // Meter ID as printed on device body
    float current_reading;
    float h0_reading;
    IzarUnitType unit_type;
    IzarAlarms alarms;
    uint8_t radio_interval;
    uint8_t random_generator;
    float remaining_battery_life;
    uint16_t h0_year;
    uint8_t h0_month;
    uint8_t h0_day;
    int16_t rssi;
};

// Callback type for decoded IZAR meter data
typedef void (*IzarDataCallback)(const IzarReading* reading);

// IZAR meter handler class
class IzarHandler {
  public:
    IzarHandler();

    // Initialize the IZAR handler
    void init();

    // Process decrypted PRIOS payload (IZAR meter data)
    bool processData(const char* meterId, const uint8_t* data, uint8_t dataLen, int16_t rssi);

    // Set callback for decoded data
    void setDataCallback(IzarDataCallback callback);

  private:
    IzarDataCallback dataCallback;

    // Parse IZAR meter reading
    bool parseReading(const char* meterId, const uint8_t* data, uint8_t dataLen, int16_t rssi, IzarReading* reading);
};

// Global IZAR handler instance
extern IzarHandler izarHandler;

#endif // IZAR_HANDLER_H
