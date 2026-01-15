#include "wm_bus_handler.h"

WmBusHandler wmBusHandler;

WmBusHandler::WmBusHandler() {}

void WmBusHandler::init() {
    LOG_INFO("wM-Bus", "Handler initialized successfully");
}

// 3-out-of-6 decoding lookup table
// Maps 6-bit patterns (with exactly 3 ones) to 4-bit values
uint8_t WmBusHandler::decode3outof6Nibble(uint8_t encoded) {
    // Standard 3-out-of-6 encoding lookup table
    switch (encoded & 0x3F) {
    case 0x16:
        return 0x0; // 010110
    case 0x0D:
        return 0x1; // 001101
    case 0x0E:
        return 0x2; // 001110
    case 0x0B:
        return 0x3; // 001011
    case 0x1C:
        return 0x4; // 011100
    case 0x19:
        return 0x5; // 011001
    case 0x1A:
        return 0x6; // 011010
    case 0x13:
        return 0x7; // 010011
    case 0x2C:
        return 0x8; // 101100
    case 0x25:
        return 0x9; // 100101
    case 0x26:
        return 0xA; // 100110
    case 0x23:
        return 0xB; // 100011
    case 0x34:
        return 0xC; // 110100
    case 0x31:
        return 0xD; // 110001
    case 0x32:
        return 0xE; // 110010
    case 0x29:
        return 0xF; // 101001
    default:
        return 0xFF; // Invalid code
    }
}

// Decode 3-out-of-6 encoded data
// Input: encoded data buffer and length
// Output: decoded data buffer and decoded length
// Returns: true on success, false on error
bool WmBusHandler::decode3outof6(const uint8_t* encoded, uint8_t encodedLen, uint8_t* decoded, uint8_t* decodedLen) {
    if (!encoded || !decoded || !decodedLen) {
        return false;
    }

    *decodedLen = 0;
    uint16_t bitBuffer = 0;
    uint8_t bitsInBuffer = 0;

    for (uint8_t i = 0; i < encodedLen; i++) {
        // Add byte to bit buffer
        bitBuffer = (bitBuffer << 8) | encoded[i];
        bitsInBuffer += 8;

        // Process all complete 6-bit groups in the buffer
        while (bitsInBuffer >= 6) {
            // Extract 6 bits
            uint8_t sixBits = (bitBuffer >> (bitsInBuffer - 6)) & 0x3F;
            bitsInBuffer -= 6;

            // Decode 6 bits to 4 bits
            uint8_t fourBits = decode3outof6Nibble(sixBits);

            if (fourBits == 0xFF) {
                LOG_DEBUG("wM-Bus", "Invalid 3-out-of-6 code: 0x%02X", sixBits);
                return false;
            }

            // Accumulate decoded nibbles into bytes
            if ((*decodedLen) % 2 == 0) {
                // High nibble
                decoded[(*decodedLen) / 2] = fourBits << 4;
            } else {
                // Low nibble
                decoded[(*decodedLen) / 2] |= fourBits;
            }
            (*decodedLen)++;
        }
    }

    // Convert nibble count to byte count
    *decodedLen = (*decodedLen + 1) / 2;

    return true;
}

// CRC-16 lookup table for wM-Bus (EN 13757 polynomial 0x3D65, non-reflected)
static const uint16_t crc16_table[256] = {
    0x0000, 0x3D65, 0x7ACA, 0x47AF, 0xF594, 0xC8F1, 0x8F5E, 0xB23B, 0xD64D, 0xEB28, 0xAC87, 0x91E2, 0x23D9, 0x1EBC,
    0x5913, 0x6476, 0x91FF, 0xAC9A, 0xEB35, 0xD650, 0x646B, 0x590E, 0x1EA1, 0x23C4, 0x47B2, 0x7AD7, 0x3D78, 0x001D,
    0xB226, 0x8F43, 0xC8EC, 0xF589, 0x1E9B, 0x23FE, 0x6451, 0x5934, 0xEB0F, 0xD66A, 0x91C5, 0xACA0, 0xC8D6, 0xF5B3,
    0xB21C, 0x8F79, 0x3D42, 0x0027, 0x4788, 0x7AED, 0x8F64, 0xB201, 0xF5AE, 0xC8CB, 0x7AF0, 0x4795, 0x003A, 0x3D5F,
    0x5929, 0x644C, 0x23E3, 0x1E86, 0xACBD, 0x91D8, 0xD677, 0xEB12, 0x3D36, 0x0053, 0x47FC, 0x7A99, 0xC8A2, 0xF5C7,
    0xB268, 0x8F0D, 0xEB7B, 0xD61E, 0x91B1, 0xACD4, 0x1EEF, 0x238A, 0x6425, 0x5940, 0xACC9, 0x91AC, 0xD603, 0xEB66,
    0x595D, 0x6438, 0x2397, 0x1EF2, 0x7A84, 0x47E1, 0x004E, 0x3D2B, 0x8F10, 0xB275, 0xF5DA, 0xC8BF, 0x23AD, 0x1EC8,
    0x5967, 0x6402, 0xD639, 0xEB5C, 0xACF3, 0x9196, 0xF5E0, 0xC885, 0x8F2A, 0xB24F, 0x0074, 0x3D11, 0x7ABE, 0x47DB,
    0xB252, 0x8F37, 0xC898, 0xF5FD, 0x47C6, 0x7AA3, 0x3D0C, 0x0069, 0x641F, 0x597A, 0x1ED5, 0x23B0, 0x918B, 0xACEE,
    0xEB41, 0xD624, 0x7A6C, 0x4709, 0x00A6, 0x3DC3, 0x8FF8, 0xB29D, 0xF532, 0xC857, 0xAC21, 0x9144, 0xD6EB, 0xEB8E,
    0x59B5, 0x64D0, 0x237F, 0x1E1A, 0xEB93, 0xD6F6, 0x9159, 0xAC3C, 0x1E07, 0x2362, 0x64CD, 0x59A8, 0x3DDE, 0x00BB,
    0x4714, 0x7A71, 0xC84A, 0xF52F, 0xB280, 0x8FE5, 0x64F7, 0x5992, 0x1E3D, 0x2358, 0x9163, 0xAC06, 0xEBA9, 0xD6CC,
    0xB2BA, 0x8FDF, 0xC870, 0xF515, 0x472E, 0x7A4B, 0x3DE4, 0x0081, 0xF508, 0xC86D, 0x8FC2, 0xB2A7, 0x009C, 0x3DF9,
    0x7A56, 0x4733, 0x2345, 0x1E20, 0x598F, 0x64EA, 0xD6D1, 0xEBB4, 0xAC1B, 0x917E, 0x475A, 0x7A3F, 0x3D90, 0x00F5,
    0xB2CE, 0x8FAB, 0xC804, 0xF561, 0x9117, 0xAC72, 0xEBDD, 0xD6B8, 0x6483, 0x59E6, 0x1E49, 0x232C, 0xD6A5, 0xEBC0,
    0xAC6F, 0x910A, 0x2331, 0x1E54, 0x59FB, 0x649E, 0x00E8, 0x3D8D, 0x7A22, 0x4747, 0xF57C, 0xC819, 0x8FB6, 0xB2D3,
    0x59C1, 0x64A4, 0x230B, 0x1E6E, 0xAC55, 0x9130, 0xD69F, 0xEBFA, 0x8F8C, 0xB2E9, 0xF546, 0xC823, 0x7A18, 0x477D,
    0x00D2, 0x3DB7, 0xC83E, 0xF55B, 0xB2F4, 0x8F91, 0x3DAA, 0x00CF, 0x4760, 0x7A05, 0x1E73, 0x2316, 0x64B9, 0x59DC,
    0xEBE7, 0xD682, 0x912D, 0xAC48};

// CRC-16 calculation for wM-Bus (EN 13757 standard)
// Poly: 0x3D65, Init: 0x0000, Final XOR: 0xFFFF, No reflection
uint16_t WmBusHandler::calculateCRC16(const uint8_t* data, uint8_t length) {
    uint16_t crc = 0x0000;

    for (uint8_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }

    return crc ^ 0xFFFF; // Final XOR
}

// Verify CRC-16
bool WmBusHandler::verifyCRC16(const uint8_t* data, uint8_t length, uint16_t expectedCRC) {
    uint16_t calculatedCRC = calculateCRC16(data, length);

    if (calculatedCRC != expectedCRC) {
        LOG_ERROR("wM-Bus", "CRC mismatch: calculated=0x%04X, expected=0x%04X", calculatedCRC, expectedCRC);
        return false;
    }

    return true;
}

// Parse wM-Bus header
bool WmBusHandler::parseDataLinkLayerHeader(const uint8_t* data, uint8_t length, WmBusDataLinkLayerHeader* header) {
    if (!data || !header || length < WM_BUS_HEADER_SIZE) {
        LOG_ERROR("wM-Bus", "Invalid Data Link Layer header length: %d (expected at least %d)", length,
                  WM_BUS_HEADER_SIZE);
        return false;
    }

    // CRC16 (2 bytes, big-endian for EN 13757 non-reflected)
    header->crcHeader = (data[WM_BUS_OFFSET_CRC] << 8) | data[WM_BUS_OFFSET_CRC + 1];

    // Verify Data Link Layer CRC (covers WM_BUS_HEADER_SIZE bytes)
    if (!verifyCRC16(data, WM_BUS_HEADER_SIZE, header->crcHeader)) {
        LOG_DEBUG("wM-Bus", "Header CRC verification failed");
        return false;
    }

    // Parse header fields directly from data buffer using offsets
    header->lField = data[WM_BUS_OFFSET_L_FIELD];
    header->cField = data[WM_BUS_OFFSET_C_FIELD];

    // Manufacturer field (2 bytes, little-endian)
    uint16_t mField = data[WM_BUS_OFFSET_M_FIELD] | (data[WM_BUS_OFFSET_M_FIELD + 1] << 8);
    // Decode 3-character manufacturer ID
    // Each character is 5 bits (A=1, B=2, ..., Z=26)
    header->manufacturer[0] = '@' + ((mField >> 10) & 0x1F);
    header->manufacturer[1] = '@' + ((mField >> 5) & 0x1F);
    header->manufacturer[2] = '@' + (mField & 0x1F);
    header->manufacturer[3] = '\0';

    // PRIOS protocol uses full A-field including version and device type to encode meter identity
    // Decode meter ID as printed on device body
    // Extract 26-bit number from A-field (bytes 4-7, using only lower 2 bits of byte 7)
    uint32_t meter_id = ((data[WM_BUS_OFFSET_A_FIELD + 3] & 0x03) << 24) | (data[WM_BUS_OFFSET_A_FIELD + 2] << 16) |
                        (data[WM_BUS_OFFSET_A_FIELD + 1] << 8) | data[WM_BUS_OFFSET_A_FIELD];

    // Meter ID is 8 decimal digits encoded in BCD within the 26-bit number
    // 2 digits for manufacture year + 6 digits for serial number
    // Extract manufacture year from first 2 digits
    uint8_t yy = (meter_id / 1000000) % 100;
    uint32_t meter_number = meter_id % 1000000;

    // Extract 3 letters from version and device type bytes
    // Each letter is encoded in 5 bits and needs '@' (64) added to get ASCII
    char supplier_code = '@' + (((data[WM_BUS_OFFSET_DEVICE_TYPE] & 0x0F) << 1) | (data[WM_BUS_OFFSET_VERSION] >> 7));
    char meter_type = '@' + ((data[WM_BUS_OFFSET_VERSION] & 0x7C) >> 2);
    char diameter = '@' + (((data[WM_BUS_OFFSET_VERSION] & 0x03) << 3) | (data[WM_BUS_OFFSET_VERSION - 1] >> 5));

    // Build meter ID as printed on device body
    snprintf(header->meterId, sizeof(header->meterId), "%c%02d%c%c%06lu", supplier_code, yy, meter_type, diameter,
             static_cast<unsigned long>(meter_number));

    LOG_DEBUG("wM-Bus", "Data Link Layer header parsed successfully:");
    LOG_DEBUG("wM-Bus", "  L-field: 0x%02X (%d bytes)", header->lField, header->lField);
    LOG_DEBUG("wM-Bus", "  C-field: 0x%02X", header->cField);
    LOG_DEBUG("wM-Bus", "  M-field: 0x%04X (%s)", header->manufacturer, mfgString);
    LOG_DEBUG("wM-Bus", "  Meter ID: %s", header->meterId);
    LOG_DEBUG("wM-Bus", "  DLL CRC: 0x%04X (valid)", header->crcHeader);

    return true;
}

// Process raw FSK modem packet (3-out-of-6 encoded)
bool WmBusHandler::processRawPacket(const uint8_t* rawData, uint8_t rawLength, int16_t rssi) {
    if (!rawData || rawLength == 0) {
        LOG_ERROR("wM-Bus", "Invalid raw packet");
        return false;
    }

    if (rawLength > WM_BUS_MAX_PAYLOAD) {
        LOG_ERROR("wM-Bus", "Packet too large: %d bytes (max %d)", rawLength, WM_BUS_MAX_PAYLOAD);
        return false;
    }

    LOG_DEBUG("wM-Bus", "Processing raw packet (%d bytes)", rawLength);
    LOG_DEBUG("wM-Bus", "Raw: ");
    for (uint8_t i = 0; i < rawLength && i < 32; i++) {
        LOG_DEBUG("wM-Bus", "%02X ", rawData[i]);
    }
    if (rawLength > 32)
        LOG_DEBUG("wM-Bus", "...");
    LOG_DEBUG("wM-Bus", "");

    // First, decode just the L-field (first byte) to determine actual packet length
    // L-field is 1 byte (8 bits = 2 nibbles), which requires 12 bits (2 × 6 bits) encoded
    // So we need at least 2 bytes of raw data to decode the L-field
    if (rawLength < 2) {
        LOG_ERROR("wM-Bus", "Raw packet too short to contain L-field");
        return false;
    }

    uint8_t lFieldDecoded[1];
    uint8_t lFieldDecodedLen = 0;

    if (!decode3outof6(rawData, 2, lFieldDecoded, &lFieldDecodedLen)) {
        LOG_ERROR("wM-Bus", "Failed to decode L-field");
        return false;
    }

    if (lFieldDecodedLen < 1) {
        LOG_ERROR("wM-Bus", "L-field decoding produced no data");
    }

    uint8_t lField = lFieldDecoded[0];
    uint8_t expectedDecodedLen =
        lField + 1 + 2 * WM_BUS_HEADER_CRC_SIZE; // L-field = total length - 1 + DLL CRC + TPL CRC

    LOG_DEBUG("wM-Bus", "L-field: 0x%02X (expected decoded length: %d bytes)", lField, expectedDecodedLen);

    // Calculate required encoded bytes for the full packet
    // 3-out-of-6 encoding: 1 decoded byte → 1.5 encoded bytes
    // encoded_bytes = decoded_bytes * 3 / 2
    uint16_t requiredEncodedBytes = (expectedDecodedLen * 3 + 1) / 2; // Round up

    LOG_DEBUG("wM-Bus", "Required encoded bytes: %d, available: %d", requiredEncodedBytes, rawLength);

    if (requiredEncodedBytes > rawLength) {
        LOG_ERROR("wM-Bus", "Insufficient raw data: need %d bytes, have %d", requiredEncodedBytes, rawLength);
        return false;
    }

    // Now decode the full packet using only the required bytes
    uint8_t decoded[WM_BUS_MAX_PAYLOAD];
    uint8_t decodedLen = 0;

    if (!decode3outof6(rawData, requiredEncodedBytes, decoded, &decodedLen)) {
        LOG_DEBUG("wM-Bus", "3-out-of-6 decoding failed");
        return false;
    }

    LOG_DEBUG("wM-Bus", "Decoded %d bytes (expected %d)", decodedLen, expectedDecodedLen);

    // Verify decoded length matches expected
    if (decodedLen < expectedDecodedLen) {
        LOG_WARN("wM-Bus", "Decoded fewer bytes than expected (%d < %d)", decodedLen, expectedDecodedLen);
    } else if (decodedLen > expectedDecodedLen) {
        LOG_WARN("wM-Bus", "Decoded more bytes than expected (%d > %d), truncating", decodedLen, expectedDecodedLen);
        decodedLen = expectedDecodedLen; // Use only expected length
    }
    LOG_DEBUG("wM-Bus", "Decoded: ");
    for (uint8_t i = 0; i < decodedLen && i < 32; i++) {
        LOG_DEBUG("wM-Bus", "%02X ", decoded[i]);
    }
    if (decodedLen > 32)
        LOG_DEBUG("wM-Bus", "...");
    LOG_DEBUG("wM-Bus", "");

    // Check minimum length for header
    if (decodedLen < WM_BUS_HEADER_SIZE + WM_BUS_HEADER_CRC_SIZE) {
        LOG_ERROR("wM-Bus", "Packet too short: %d bytes (need at least %d)", decodedLen,
                  WM_BUS_HEADER_SIZE + WM_BUS_HEADER_CRC_SIZE);
        return false;
    }

    // Parse header
    WmBusDataLinkLayerHeader header;
    if (!parseDataLinkLayerHeader(decoded, decodedLen, &header)) {
        LOG_ERROR("wM-Bus", "Data Link Layer header parsing failed");
        return false;
    }

    // Extract Transport Layer data (after DLL header + DLL CRC, before TPL CRC)
    uint8_t tplTotalLen = decodedLen - (WM_BUS_HEADER_SIZE + WM_BUS_HEADER_CRC_SIZE);
    uint8_t* tplData = decoded + WM_BUS_HEADER_SIZE + WM_BUS_HEADER_CRC_SIZE;

    // Check for TPL CRC (last 2 bytes of TPL data)
    if (tplTotalLen < WM_BUS_HEADER_CRC_SIZE) {
        LOG_ERROR("wM-Bus", "No Transport Layer data");
        return false;
    }

    uint8_t tplDataLen = tplTotalLen - WM_BUS_HEADER_CRC_SIZE;
    uint16_t tplCrc = (tplData[tplDataLen] << 8) | tplData[tplDataLen + 1];

    // Verify Transport Layer CRC
    if (!verifyCRC16(tplData, tplDataLen, tplCrc)) {
        LOG_ERROR("wM-Bus", "Transport Layer CRC verification failed");
        return false;
    }

    LOG_DEBUG("wM-Bus", "Transport Layer CRC: 0x%04X (valid)", tplCrc);

    if (tplDataLen > 0) {
        LOG_DEBUG("wM-Bus", "Transport Layer data (%d bytes)", tplDataLen);
    } else {
        LOG_DEBUG("wM-Bus", "No Transport Layer data");
    }

    // Call user callback if registered (pass full decoded frame and TPL data without CRC)
    if (packetCallback != nullptr) {
        packetCallback(header.meterId, decoded, decodedLen, tplData, tplDataLen, rssi);
    }

    return true;
}

// Set packet callback
void WmBusHandler::setPacketCallback(WmBusPacketCallback callback) {
    packetCallback = callback;
}
