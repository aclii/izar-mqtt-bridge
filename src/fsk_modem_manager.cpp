#include "fsk_modem_manager.h"
#include "gpio_expander_manager.h"

FskModemManager fskModemManager;

// Define static member
volatile bool FskModemManager::packetAvailable = false;

FskModemManager::FskModemManager()
    : radioModule(nullptr), radio(nullptr), packetBuffer{0}, packetLength(0), lastRSSI(0) {}

bool FskModemManager::init(SPIClass* sharedSPI) {
    if (!sharedSPI) {
        LOG_ERROR("FSK Modem", "Error: Shared SPI not provided");
        return false;
    }

    spi = sharedSPI;
    LOG_DEBUG("FSK Modem", "Initializing SX1262 module...");
    LOG_DEBUG("FSK Modem", "SPI pins: CS=%d, IRQ=%d, BUSY=%d", SX1262_CS_PIN, SX1262_IRQ_PIN, SX1262_BUSY_PIN);

    // Create Module with shared SPI bus and SPI settings
    spiSettings = SPISettings(SX1262_SSD1306_SPI_FREQUENCY, MSBFIRST, SPI_MODE0);
    radioModule = new Module(SX1262_CS_PIN, SX1262_IRQ_PIN, RADIOLIB_NC, SX1262_BUSY_PIN, *spi, spiSettings);
    radio = new SX1262(radioModule);

    // Configure SX1262 control pins via GPIO expander
    // Enable antenna amplifier (LNA)
    gpioExpanderManager.setSXLNAEnabled(true);

    // Enable antenna RF switch
    gpioExpanderManager.setSXAntennaSwitched(true);

    // Release SX1262 from reset (set RST high)
    gpioExpanderManager.setSXReset(true);
    delay(100); // Wait for reset

    // Initialize SX1262 in FSK mode
    int state = radio->beginFSK();
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Initialization failed with error code: %d", state);
        LOG_ERROR("FSK Modem", "Error details: %s",
                  state == RADIOLIB_ERR_CHIP_NOT_FOUND ? "Chip Not Found" : "Unknown Error");
        return false;
    }

    state = radio->setFrequency(FSK_MODEM_FREQUENCY);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set frequency: %d", state);
        return false;
    }

    state = radio->setBitRate(FSK_MODEM_BIT_RATE);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set bit rate: %d", state);
        return false;
    }

    state = radio->setFrequencyDeviation(FSK_MODEM_FREQUENCY_DEVIATION);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set frequency deviation: %d", state);
        return false;
    }

    state = radio->setRxBandwidth(FSK_MODEM_RX_BANDWIDTH);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set RX bandwidth: %d", state);
        return false;
    }

    uint8_t sync_word[] = FSK_MODEM_SYNC_WORD;
    state = radio->setSyncWord(sync_word, sizeof(sync_word));
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set sync word: %d", state);
        return false;
    }

    // Variable packet length mode uses first received byte as length
    // thus it dissapers from the payload and getPacketLength returns incorrect value
    // due to 3-out-of-6 encoding expanding each 6 bits to 8 bits, we set fixed length to 255 bytes
    // and will handle actual length in software after decoding
    state = radio->fixedPacketLengthMode(FSK_MODEM_RX_MAX_LENGTH);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set fixed packet length mode: %d", state);
        return false;
    }

    state = radio->setTCXO(3.0); // Set TCXO voltage to 3.0V (from Unit-C6Lschematics)
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set TCXO voltage: %d", state);
        return false;
    }

    state = radio->setRegulatorDCDC(); // Use DC-DC regulator
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set regulator mode to DC/DC: %d", state);
        return false;
    }

    // Disable data shaping and set NRZ encoding
    state = radio->setDataShaping(RADIOLIB_SHAPING_NONE);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set data shaping: %d", state);
        return false;
    }

    state = radio->setEncoding(RADIOLIB_ENCODING_NRZ);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to set encoding: %d", state);
        return false;
    }

    // Disable CRC, wM-Bus uses its own checksum
    state = radio->setCRC(0);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to disable CRC: %d", state);
        return false;
    }

    // Configure DIO2 as RF switch control
    radio->setDio2AsRfSwitch(true);

    // Set interrupt handler
    radio->setPacketReceivedAction(ReceiveInterruptHandler);

    // Start receiving
    state = radio->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FSK Modem", "Failed to start receive: %d", state);
        return false;
    }

    LOG_INFO("FSK Modem", "Initialized successfully");
    LOG_DEBUG("FSK Modem", "Frequency: %.2f MHz, Frequency Deviation +/- %.1f kHz", FSK_MODEM_FREQUENCY,
              FSK_MODEM_FREQUENCY_DEVIATION);
    LOG_DEBUG("FSK Modem", "Bitrate: %.1f kbps, Rx Bandwidth: %.1f kHz", FSK_MODEM_BIT_RATE, FSK_MODEM_RX_BANDWIDTH);

    return true;
}

void FskModemManager::receive() {
    if (!packetAvailable || !radio) {
        return;
    }

    packetAvailable = false;

    int state = radio->readData(packetBuffer, FSK_MODEM_RX_MAX_LENGTH);

    if (state == RADIOLIB_ERR_NONE) {
        lastRSSI = radio->getRSSI();

        if (externalCallback != nullptr) {
            externalCallback(packetBuffer, FSK_MODEM_RX_MAX_LENGTH, lastRSSI);
        }
    } else {
        LOG_ERROR("FSK Modem", "Error reading data: %d", state);
    }

    // Resume receiving
    radio->startReceive();
}

void FskModemManager::handle() {
    receive();
}

// Static interrupt handler - called from ISR
void FskModemManager::ReceiveInterruptHandler() {
    fskModemManager.packetAvailable = true;
}

void FskModemManager::setCallback(FskModemCallbackFunction callback) {
    externalCallback = callback;
}
