#ifndef FSK_MODEM_MANAGER_H
#define FSK_MODEM_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "config.h"

typedef void (*FskModemCallbackFunction)(const uint8_t* payload, uint8_t length, int rssi);

class FskModemManager {
  private:
    Module* radioModule;     // RadioLib Module
    SX1262* radio;           // SX1262 FSK modem instance
    SPIClass* spi = nullptr; // Shared SPI bus
    SPISettings spiSettings; // SPI configuration
    FskModemCallbackFunction externalCallback = nullptr;
    static volatile bool packetAvailable;
    uint8_t packetBuffer[FSK_MODEM_RX_MAX_LENGTH];
    volatile int packetLength = 0;
    int lastRSSI = 0;

    // Non-static receive handler
    void receive();

    // Static callback for interrupt
    static void ReceiveInterruptHandler();

  public:
    FskModemManager();

    bool init(SPIClass* sharedSPI);
    void handle();

    // Callbacks
    void setCallback(FskModemCallbackFunction callback);
};

extern FskModemManager fskModemManager;

#endif // FSK_MODEM_MANAGER_H
