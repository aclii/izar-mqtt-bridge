#ifndef PRIOS_HANDLER_H
#define PRIOS_HANDLER_H

#include <Arduino.h>
#include "config.h"

// PRIOS protocol constants
#define PRIOS_KEY_SIZE 8 // 8-byte key for LFSR

// IZAR default encryption keys
// clang-format off
#define PRIOS_DEFAULT_KEY1 {0x39, 0xBC, 0x8A, 0x10, 0xE6, 0x6D, 0x83, 0xF8}
#define PRIOS_DEFAULT_KEY2 {0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8}
// clang-format on
#define PRIOS_DEFAULT_KEY PRIOS_DEFAULT_KEY1

#define PRIOS_OFFSET_ENCRYPTED_DATA 5 // Start of encrypted data

// PRIOS handler class
class PriosHandler {
  public:
    PriosHandler();

    // Initialize the PRIOS handler
    void init();

    // Process wM-Bus payload (PRIOS encrypted data)
    bool processPayload(const char* meterId, const uint8_t* fullFrame, uint8_t fullFrameLen, uint8_t* payload,
                        uint8_t payloadLen, int16_t rssi);

  private:
    // Decrypt PRIOS data using LFSR (in-place decryption)
    bool decryptData(const uint8_t* key, uint8_t* data, uint8_t dataLen, const uint8_t* frame);
};

// Global PRIOS handler instance
extern PriosHandler priosHandler;

#endif // PRIOS_HANDLER_H
