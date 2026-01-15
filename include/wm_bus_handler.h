#ifndef WM_BUS_HANDLER_H
#define WM_BUS_HANDLER_H

#include <Arduino.h>
#include "config.h"

// wM-Bus packet structure constants
#define WM_BUS_HEADER_SIZE                                                                                             \
    10                                             // wM-Bus header (Data Link Layer) size without CRC
                                                   // L-field encodes number of data bytes excluding CRCs
                                                   // Data Link Layer CRC is 2 bytes and Transport Layer CRC is 2 bytes
                                                   // so total packet size is L-field + 4 + 1 (L-field itself)
#define WM_BUS_HEADER_CRC_SIZE 2                   // wM-Bus header (Data Link Layer) CRC-16 size
#define WM_BUS_MAX_PAYLOAD FSK_MODEM_RX_MAX_LENGTH // Maximum payload size

// wM-Bus header field offsets
#define WM_BUS_OFFSET_L_FIELD 0     // Length field
#define WM_BUS_OFFSET_C_FIELD 1     // Control field
#define WM_BUS_OFFSET_M_FIELD 2     // Manufacturer field (2 bytes)
#define WM_BUS_OFFSET_A_FIELD 4     // Address field start (6 bytes)
#define WM_BUS_OFFSET_VERSION 8     // Version byte
#define WM_BUS_OFFSET_DEVICE_TYPE 9 // Device type byte
#define WM_BUS_OFFSET_CRC 10        // CRC-16 (2 bytes, big-endian)

// Callback for successfully parsed wM-Bus packets
typedef void (*WmBusPacketCallback)(const char* meterId, const uint8_t* fullwMBusFrame, uint8_t fullwMBusFrameLen,
                                    uint8_t* tplData, uint8_t tplDataLen, int16_t rssi);

// wM-Bus header structure (Mode T1) - stores parsed values only
struct WmBusDataLinkLayerHeader {
    char meterId[16];     // String meter ID (as printed on the meter)
    char manufacturer[4]; // Manufacturer code (parsed from bytes 2-3)
    uint16_t crcHeader;   // CRC-16 for header (parsed from bytes 10-11, big-endian)
    uint8_t lField;       // Length field (byte 0)
    uint8_t cField;       // Control field (byte 1)
};

class WmBusHandler {
  private:
    WmBusPacketCallback packetCallback = nullptr;

    // 3-out-of-6 decoding
    uint8_t decode3outof6Nibble(uint8_t encoded);
    bool decode3outof6(const uint8_t* encoded, uint8_t encodedLen, uint8_t* decoded, uint8_t* decodedLen);

    // CRC calculation
    uint16_t calculateCRC16(const uint8_t* data, uint8_t length);
    bool verifyCRC16(const uint8_t* data, uint8_t length, uint16_t expectedCRC);

    // Header parsing
    bool parseDataLinkLayerHeader(const uint8_t* data, uint8_t length, WmBusDataLinkLayerHeader* header);

  public:
    WmBusHandler();

    // Initialize handler
    void init();

    // Process raw FSK modem data (3-out-of-6 encoded)
    bool processRawPacket(const uint8_t* rawData, uint8_t rawLength, int16_t rssi);

    // Set callback for parsed packets
    void setPacketCallback(WmBusPacketCallback callback);
};

extern WmBusHandler wmBusHandler;

#endif // WM_BUS_HANDLER_H
