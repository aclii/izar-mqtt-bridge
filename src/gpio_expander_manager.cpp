#include "gpio_expander_manager.h"

GPIOExpanderManager gpioExpanderManager;
volatile bool GPIOExpanderManager::interruptFlag = false;

GPIOExpanderManager::GPIOExpanderManager() {}

bool GPIOExpanderManager::init() {
    LOG_DEBUG("GPIOExpander", "Initializing...");

    // Initialize I2C on the specified pins
    Wire.begin(GPIO_EXPANDER_SDA_PIN, GPIO_EXPANDER_SCL_PIN);
    Wire.setClock(GPIO_EXPANDER_I2C_CLOCK);
    delay(100);

    // Check if device is responding
    Wire.beginTransmission(deviceAddress);
    int error = Wire.endTransmission();

    if (error != 0) {
        LOG_ERROR("GPIOExpander", "Device not found at address 0x%02X", deviceAddress);
        return false;
    }

    // Configure pin directions (1 = output, 0 = input)
    if (!writeRegister(REG_CONFIG, CONFIG_MASK)) {
        LOG_ERROR("GPIOExpander", "Failed to configure pins");
        return false;
    }

    // Enable pull-up resistors for input pins
    if (!writeRegister(REG_PULL_ENABLE, uint8_t(~CONFIG_MASK))) {
        LOG_ERROR("GPIOExpander", "Failed to enable pull resistors");
        return false;
    }

    if (!writeRegister(REG_PULL_SELECT, uint8_t(~CONFIG_MASK))) {
        LOG_ERROR("GPIOExpander", "Failed to select pull-up");
        return false;
    }

    // Set default input states active low for interupt detection
    if (!writeRegister(REG_INPUT_DEFAULT, uint8_t(~CONFIG_MASK))) {
        LOG_ERROR("GPIOExpander", "Failed to select input default states");
        return false;
    }

    // Initialize output port state
    if (!writeRegister(REG_OUTPUT, portState)) {
        LOG_ERROR("GPIOExpander", "Failed to initialize outputs");
        return false;
    }

    // Enable outputs, turn off high-impedance on selected outputs
    if (!writeRegister(REG_HIGH_Z, uint8_t(~CONFIG_MASK))) {
        LOG_ERROR("GPIOExpander", "Failed to enable outputs");
        return false;
    }

    // Enable interrupts for SYS_KEY1 button - 0 = interrupt enabled, 1 = masked
    if (!writeRegister(REG_INT_MASK, ~MASK_SYS_KEY1)) {
        LOG_ERROR("GPIOExpander", "Failed to set interrupt mask");
        return false;
    }

    // Configure interrupt pin as input (external pull-up on hardware)
    if (GPIO_EXPANDER_INT_PIN >= 0) {
        pinMode(GPIO_EXPANDER_INT_PIN, INPUT);
    }

    // Attach interrupt handler to ESP32 GPIO pin
    if (GPIO_EXPANDER_INT_PIN >= 0) {
        interruptFlag = false;
        attachInterrupt(digitalPinToInterrupt(GPIO_EXPANDER_INT_PIN), handleInterrupt, FALLING);
        LOG_INFO("GPIOExpander", "Button interrupt enabled on pin %d", GPIO_EXPANDER_INT_PIN);
    }

    // Clear any pending interrupts - read status register to clear, and read device ID to clear the internal state
    uint8_t status;
    readRegister(REG_INT_STATUS, status);
    readRegister(REG_DEVICE_ID, status);

    initialized = true;
    LOG_INFO("GPIOExpander", "Manager initialized successfully");

    return true;
}

bool GPIOExpanderManager::reinit() {
    LOG_INFO("GPIOExpander", "Re-initializing I2C bus...");

    // Reset I2C bus
    Wire.end();
    delay(100);

    // Re-initialize I2C
    Wire.begin(GPIO_EXPANDER_SDA_PIN, GPIO_EXPANDER_SCL_PIN);
    Wire.setClock(GPIO_EXPANDER_I2C_CLOCK);
    delay(100);

    // Check if device is responding
    Wire.beginTransmission(deviceAddress);
    int error = Wire.endTransmission();

    if (error != 0) {
        LOG_ERROR("GPIOExpander", "Device not found after reinit at address 0x%02X", deviceAddress);
        initialized = false;
        return false;
    }

    // Reconfigure the device
    writeRegister(REG_CONFIG, CONFIG_MASK);
    writeRegister(REG_PULL_ENABLE, uint8_t(~CONFIG_MASK));
    writeRegister(REG_PULL_SELECT, uint8_t(~CONFIG_MASK));
    writeRegister(REG_HIGH_Z, uint8_t(~CONFIG_MASK));
    writeRegister(REG_OUTPUT, portState);

    LOG_DEBUG("GPIOExpander", "Re-initialization complete");
    consecutiveErrors = 0;
    return true;
}

bool GPIOExpanderManager::readRegister(uint8_t reg, uint8_t& value) {
    // Add small delay to prevent I2C bus overload
    delayMicroseconds(100);

    Wire.beginTransmission(deviceAddress);
    Wire.write(reg);
    int error = Wire.endTransmission(false); // Don't release bus

    if (error != 0) {
        LOG_ERROR("GPIOExpander", "Read error on reg 0x%02X: %d", reg, error);
        consecutiveErrors++;
        if (consecutiveErrors >= MAX_ERRORS_BEFORE_RESET) {
            LOG_ERROR("GPIOExpander", "Too many errors, attempting recovery...");
            reinit();
        }
        return false;
    }

    // Request with timeout
    uint8_t received = Wire.requestFrom(deviceAddress, (uint8_t)1);
    if (received != 1) {
        consecutiveErrors++;
        if (consecutiveErrors >= MAX_ERRORS_BEFORE_RESET) {
            LOG_ERROR("GPIOExpander", "Too many errors (%d), attempting recovery...", consecutiveErrors);
            reinit();
            consecutiveErrors = 0;
        }
        return false;
    }

    value = Wire.read();
    consecutiveErrors = 0; // Reset error counter on success
    return true;
}

bool GPIOExpanderManager::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(deviceAddress);
    Wire.write(reg);
    Wire.write(value);
    int error = Wire.endTransmission();

    if (error != 0) {
        LOG_ERROR("GPIOExpander", "Write error to reg 0x%02X: %d", reg, error);
        return false;
    }

    return true;
}

// Static interrupt handler - MUST be minimal, no logging!
void IRAM_ATTR GPIOExpanderManager::handleInterrupt() {
    interruptFlag = true;
}

ButtonEvent GPIOExpanderManager::getButtonEvent() {
    if (!initialized)
        return ButtonEvent::BUTTON_EVENT_NONE;

    // Check for button press interrupt
    if (interruptFlag && !buttonPressed) {
        interruptFlag = false;
        // Read interrupt status to clear the interrupt
        uint8_t status;
        if (!readRegister(REG_INT_STATUS, status)) {
            LOG_ERROR("GPIOExpander", "Failed to read interrupt status");
            return ButtonEvent::BUTTON_EVENT_NONE;
        }

        // Check if button interrupt occurred
        if ((status & MASK_SYS_KEY1) != 0) {
            buttonPressed = true;
            buttonPressStartTime = millis();
            LOG_DEBUG("GPIOExpander", "Button pressed");
        }
    }

    // Poll button state if button was pressed
    if (buttonPressed) {
        // Read current input pin state
        uint8_t inputState;
        if (!readRegister(REG_INPUT, inputState)) {
            LOG_ERROR("GPIOExpander", "Failed to read input state");
            return ButtonEvent::BUTTON_EVENT_NONE;
        }

        // Button is active low (pressed = 0, released = 1)
        bool currentlyPressed = ((inputState & MASK_SYS_KEY1) == 0);

        if (!currentlyPressed) {
            // Button released - determine event type
            buttonPressed = false;
            unsigned long pressDuration = millis() - buttonPressStartTime;

            if (pressDuration >= LONG_PRESS_DURATION) {
                LOG_DEBUG("GPIOExpander", "Long press detected (%lu ms)", pressDuration);
                return ButtonEvent::BUTTON_EVENT_LONG_PRESS;
            } else {
                LOG_DEBUG("GPIOExpander", "Short press detected (%lu ms)", pressDuration);
                return ButtonEvent::BUTTON_EVENT_SHORT_PRESS;
            }
        }
    }

    return ButtonEvent::BUTTON_EVENT_NONE;
}

void GPIOExpanderManager::setSXLNAEnabled(bool enabled) {
    if (!initialized)
        return;

    if (enabled) {
        portState |= MASK_SX_LNA_EN;
    } else {
        portState &= ~MASK_SX_LNA_EN;
    }

    writeRegister(REG_OUTPUT, portState);
    LOG_DEBUG("GPIOExpander", "SX_LNA_EN: %s", enabled ? "enabled" : "disabled");
}

void GPIOExpanderManager::setSXAntennaSwitched(bool enabled) {
    if (!initialized)
        return;

    if (enabled) {
        portState |= MASK_SX_ANT_SW;
    } else {
        portState &= ~MASK_SX_ANT_SW;
    }

    writeRegister(REG_OUTPUT, portState);
    LOG_DEBUG("GPIOExpander", "SX_ANT_SW: %s", enabled ? "enabled" : "disabled");
}

void GPIOExpanderManager::setSXReset(bool state) {
    if (!initialized)
        return;

    if (state) {
        portState |= MASK_SX_NRST;
    } else {
        portState &= ~MASK_SX_NRST;
    }

    writeRegister(REG_OUTPUT, portState);
    LOG_DEBUG("GPIOExpander", "SX_NRST: %s", state ? "high (normal)" : "low (reset)");
}
