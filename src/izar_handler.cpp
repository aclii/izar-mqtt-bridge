#include "izar_handler.h"

IzarHandler izarHandler;

IzarHandler::IzarHandler() : dataCallback(nullptr) {}

static inline uint32_t readUint32LE(const uint8_t* data, uint8_t offset) {
    return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

void IzarHandler::init() {
    LOG_INFO("IZAR", "Handler initialized successfully");
}

// Parse IZAR meter data into reading structure
bool IzarHandler::parseReading(const char* meterId, const uint8_t* data, uint8_t dataLen, int16_t rssi,
                               IzarReading* reading) {
    if (!data || !reading || dataLen < IZAR_MIN_DATA_LENGTH) {
        LOG_ERROR("IZAR", "Insufficient data for parsing");
        return false;
    }

    // Store meter ID and signal quality
    strncpy(reading->meterId, meterId, sizeof(reading->meterId) - 1);
    reading->meterId[sizeof(reading->meterId) - 1] = '\0';
    reading->rssi = rssi;

    // Extract status byte 0: radio interval and random generator
    reading->radio_interval = 1 << ((data[IZAR_OFFSET_STATUS_0] & 0x0F) + 2);
    reading->random_generator = (data[IZAR_OFFSET_STATUS_0] >> 4) & 0x3;

    // Extract battery remaining life (5 bits, divide by 2 for years)
    reading->remaining_battery_life = (data[IZAR_OFFSET_STATUS_1] & 0x1F) / 2.0;

    // Extract alarms from status bytes
    reading->alarms.general_alarm = data[IZAR_OFFSET_STATUS_0] >> 7;
    reading->alarms.leakage_currently = data[IZAR_OFFSET_STATUS_1] >> 7;
    reading->alarms.leakage_previously = (data[IZAR_OFFSET_STATUS_1] >> 6) & 0x1;
    reading->alarms.meter_blocked = (data[IZAR_OFFSET_STATUS_1] >> 5) & 0x1;
    reading->alarms.back_flow = data[IZAR_OFFSET_STATUS_2] >> 7;
    reading->alarms.underflow = (data[IZAR_OFFSET_STATUS_2] >> 6) & 0x1;
    reading->alarms.overflow = (data[IZAR_OFFSET_STATUS_2] >> 5) & 0x1;
    reading->alarms.submarine = (data[IZAR_OFFSET_STATUS_2] >> 4) & 0x1;
    reading->alarms.sensor_fraud_currently = (data[IZAR_OFFSET_STATUS_2] >> 3) & 0x1;
    reading->alarms.sensor_fraud_previously = (data[IZAR_OFFSET_STATUS_2] >> 2) & 0x1;
    reading->alarms.mechanical_fraud_currently = (data[IZAR_OFFSET_STATUS_2] >> 1) & 0x1;
    reading->alarms.mechanical_fraud_previously = data[IZAR_OFFSET_STATUS_2] & 0x1;

    // Read current and h0 readings (32-bit little-endian, native format)
    reading->current_reading = readUint32LE(data, IZAR_OFFSET_CURRENT_READING);
    reading->h0_reading = readUint32LE(data, IZAR_OFFSET_H0_READING);

    // Extract measurement unit and apply multiplier
    uint8_t unit_type = data[IZAR_OFFSET_UNITS] >> 3;
    reading->unit_type = (unit_type == 0x02) ? VOLUME_CUBIC_METER : UNKNOWN_UNIT;

    int8_t multiplier_exponent = (data[IZAR_OFFSET_UNITS] & 0x07) - 6;
    if (multiplier_exponent > 0) {
        float multiplier = pow(10, multiplier_exponent);
        reading->current_reading *= multiplier;
        reading->h0_reading *= multiplier;
    } else if (multiplier_exponent < 0) {
        float divisor = pow(10, -multiplier_exponent);
        reading->current_reading /= divisor;
        reading->h0_reading /= divisor;
    }

    // Extract H0 date
    reading->h0_day = data[IZAR_OFFSET_H0_DATE_DAY] & 0x1F;
    reading->h0_month = data[IZAR_OFFSET_H0_DATE_MONTH] & 0xF;
    reading->h0_year = ((data[IZAR_OFFSET_H0_DATE_MONTH] & 0xF0) >> 1) + ((data[IZAR_OFFSET_H0_DATE_DAY] & 0xE0) >> 5);
    if (reading->h0_year > 80) {
        reading->h0_year += 1900;
    } else {
        reading->h0_year += 2000;
    }

    return true;
}

// Process decrypted PRIOS data (IZAR meter format)
bool IzarHandler::processData(const char* meterId, const uint8_t* data, uint8_t dataLen, int16_t rssi) {
    if (!data || dataLen < IZAR_MIN_DATA_LENGTH) {
        LOG_ERROR("IZAR", "Invalid data");
        return false;
    }

    // Magic byte should be 0x4B (magic marker from PRIOS decryption)
    if (data[IZAR_OFFSET_MAGIC_BYTE] != 0x4B) {
        LOG_ERROR("IZAR", "Invalid magic byte: 0x%02X (expected 0x4B)", data[IZAR_OFFSET_MAGIC_BYTE]);
        return false;
    }

    LOG_DEBUG("IZAR", "Processing meter data for ID %s (%d bytes, RSSI=%d dBm)", meterId, dataLen, rssi);
    LOG_DEBUG("IZAR", "Data: ");
    for (uint8_t i = 0; i < dataLen && i < 32; i++) {
        LOG_DEBUG("IZAR", "%02X ", data[i]);
    }
    if (dataLen > 32)
        LOG_DEBUG("IZAR", "...");
    LOG_DEBUG("IZAR", "");

    // Parse the meter reading
    IzarReading reading;
    if (!parseReading(meterId, data, dataLen, rssi, &reading)) {
        LOG_ERROR("IZAR", "Failed to parse meter reading");
        return false;
    }

    // Log parsed data
    LOG_INFO("IZAR", "Meter ID: %s", reading.meterId);
    LOG_INFO("IZAR", "Current reading: %.3f m³", reading.current_reading);
    LOG_INFO("IZAR", "History checkpoint reading: %.3f m³", reading.h0_reading);
    LOG_INFO("IZAR", "History checkpoint date: %04d-%02d-%02d", reading.h0_year, reading.h0_month, reading.h0_day);
    LOG_INFO("IZAR", "Radio interval: %d seconds", reading.radio_interval);
    LOG_INFO("IZAR", "Battery life: %.1f years", reading.remaining_battery_life);
    LOG_INFO("IZAR", "Alarms: general=%d, leakage=%d, blocked=%d, backflow=%d", reading.alarms.general_alarm,
             reading.alarms.leakage_currently, reading.alarms.meter_blocked, reading.alarms.back_flow);

    // Call user callback if registered
    if (dataCallback != nullptr) {
        dataCallback(&reading);
    }

    return true;
}

// Set callback for decoded data
void IzarHandler::setDataCallback(IzarDataCallback callback) {
    dataCallback = callback;
}
