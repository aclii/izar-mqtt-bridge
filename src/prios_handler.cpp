#include "prios_handler.h"
#include "izar_handler.h"

PriosHandler priosHandler;

PriosHandler::PriosHandler() {}

void PriosHandler::init() {
    LOG_INFO("PRIOS", "Handler initialized successfully");
}

// Helper: read 32-bit big-endian value from buffer
static inline uint32_t readUint32BE(const uint8_t* data, uint8_t offset) {
    return (static_cast<uint32_t>(data[offset]) << 24) | (static_cast<uint32_t>(data[offset + 1]) << 16) |
           (static_cast<uint32_t>(data[offset + 2]) << 8) | static_cast<uint32_t>(data[offset + 3]);
}

// Initialize LFSR key from 8-byte encryption key and wM-Bus frame header
static uint32_t initializeLfsrKey(const uint8_t* key, const uint8_t* frame) {
    // Convert 8-byte key to 32-bit seed (read as two big-endian 32-bit values and XOR)
    uint32_t key1 = readUint32BE(key, 0);
    uint32_t key2 = readUint32BE(key, 4);
    uint32_t lfsrKey = key1 ^ key2;

    // XOR with frame header values for frame-specific initialization
    lfsrKey ^= readUint32BE(frame, 2);  // manufacturer + address[0-1]
    lfsrKey ^= readUint32BE(frame, 6);  // address[2-3] + version + type
    lfsrKey ^= readUint32BE(frame, 12); // ci + status + ...

    return lfsrKey;
}

// Decrypt PRIOS data using LFSR stream cipher (Diehl/IZAR algorithm) - in-place
bool PriosHandler::decryptData(const uint8_t* key, uint8_t* data, uint8_t dataLen, const uint8_t* frame) {
    if (!key || !data || !frame) {
        LOG_ERROR("PRIOS", "Decryption failed: invalid parameters");
        return false;
    }

    // Initialize LFSR key from encryption key and frame header
    uint32_t lfsrKey = initializeLfsrKey(key, frame);

    // Decrypt using LFSR (in-place)
    for (uint8_t i = 0; i < dataLen; i++) {
        // Evolve LFSR for 8 iterations (one byte)
        for (uint8_t j = 0; j < 8; j++) {
            // Calculate new bit using XOR of specific bit positions
            // Taps at bits 1, 2, 11, 31 (0-indexed)
            uint8_t bit = ((lfsrKey >> 1) & 1) ^ ((lfsrKey >> 2) & 1) ^ ((lfsrKey >> 11) & 1) ^ ((lfsrKey >> 31) & 1);
            // Shift left and insert new bit at position 0
            lfsrKey = (lfsrKey << 1) | bit;
        }

        // XOR encrypted byte with low 8 bits of current LFSR state (in-place)
        data[i] ^= (lfsrKey & 0xFF);

        // Validate first byte should be 0x4B (magic marker for valid decryption)
        if (i == 0 && data[0] != 0x4B) {
            LOG_DEBUG("PRIOS", "Decryption validation failed: first byte is 0x%02X (expected 0x4B)", data[0]);
            return false;
        }
    }

    LOG_DEBUG("PRIOS", "Successfully decrypted %d bytes using LFSR", dataLen);
    return true;
}

// Process wM-Bus payload (PRIOS encrypted data)
bool PriosHandler::processPayload(const char* meterId, const uint8_t* fullwMBusFrame, uint8_t fullwMBusFrameLen,
                                  uint8_t* payload, uint8_t payloadLen, int16_t rssi) {
    if (!fullwMBusFrame || !payload || payloadLen < PRIOS_OFFSET_ENCRYPTED_DATA || fullwMBusFrameLen < 12) {
        LOG_ERROR("PRIOS", "Invalid frame or payload");
        return false;
    }

    LOG_DEBUG("PRIOS", "Processing payload (%d bytes) for meter %s (RSSI=%d dBm)", payloadLen, meterId, rssi);

    // Calculate encrypted data length
    uint8_t encryptedLen = payloadLen - PRIOS_OFFSET_ENCRYPTED_DATA;
    uint8_t* encryptedData = payload + PRIOS_OFFSET_ENCRYPTED_DATA;

    // Use default PRIOS key for decryption
    const uint8_t defaultKey[PRIOS_KEY_SIZE] = PRIOS_DEFAULT_KEY;

    // Decrypt data in-place using full wM-Bus frame for LFSR initialization
    if (!decryptData(defaultKey, encryptedData, encryptedLen, fullwMBusFrame)) {
        LOG_ERROR("PRIOS", "Decryption failed");
        return false;
    }

    LOG_DEBUG("PRIOS", "Decrypted data: ");
    for (uint8_t i = 0; i < encryptedLen && i < 32; i++) {
        LOG_DEBUG("PRIOS", "%02X ", encryptedData[i]);
    }
    if (encryptedLen > 32)
        LOG_DEBUG("PRIOS", "...");
    LOG_DEBUG("PRIOS", "");

    // Pass payload with plain and decrypted data to IZAR handler for meter-specific parsing
    return izarHandler.processData(meterId, payload, payloadLen, rssi);
}
