#ifndef GPIO_EXPANDER_MANAGER_H
#define GPIO_EXPANDER_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// PI4IOE5V6408 I2C GPIO Expander
// Controls: User Button, SX_LNA_EN, SX_ANT_SW, SX_NRST

// Button event types
enum class ButtonEvent { BUTTON_EVENT_NONE, BUTTON_EVENT_SHORT_PRESS, BUTTON_EVENT_LONG_PRESS };

class GPIOExpanderManager {
  private:
    uint8_t deviceAddress = GPIO_EXPANDER_I2C_ADDR;
    uint8_t portState = 0x00; // All low by default
    bool initialized = false;
    uint8_t consecutiveErrors = 0;
    static constexpr uint8_t MAX_ERRORS_BEFORE_RESET = 10;

    // Interrupt handling
    static volatile bool interruptFlag;
    static void IRAM_ATTR handleInterrupt();

    // Button state tracking
    bool buttonPressed = false;
    unsigned long buttonPressStartTime = 0;
    static constexpr unsigned long LONG_PRESS_DURATION = 2000; // 2 seconds

    // Register addresses for PI4IOE5V6408
    static constexpr uint8_t REG_DEVICE_ID = 0x01;     // Device ID and Control
    static constexpr uint8_t REG_CONFIG = 0x03;        // I/O Direction (0=input, 1=output)
    static constexpr uint8_t REG_OUTPUT = 0x05;        // Output Port Register
    static constexpr uint8_t REG_HIGH_Z = 0x07;        // Output High-Impedance
    static constexpr uint8_t REG_INPUT_DEFAULT = 0x09; // Input Default State
    static constexpr uint8_t REG_PULL_ENABLE = 0x0B;   // Pull-Up/Down Enable
    static constexpr uint8_t REG_PULL_SELECT = 0x0D;   // Pull-Up/Down Select
    static constexpr uint8_t REG_INPUT = 0x0F;         // Input Status (current input levels)
    static constexpr uint8_t REG_INT_MASK = 0x11;      // Interrupt Mask
    static constexpr uint8_t REG_INT_STATUS = 0x13;    // Interrupt Status

    // Bit masks for expander pins (generated from config.h defines)
    static constexpr uint8_t MASK_SYS_KEY1 = (1 << GPIO_EXPANDER_IO_SYS_KEY1);
    static constexpr uint8_t MASK_SX_LNA_EN = (1 << GPIO_EXPANDER_IO_SX_LNA_EN);
    static constexpr uint8_t MASK_SX_ANT_SW = (1 << GPIO_EXPANDER_IO_SX_ANT_SW);
    static constexpr uint8_t MASK_SX_NRST = (1 << GPIO_EXPANDER_IO_SX_NRST);

    // Configuration mask: P0 as input (0), P5/P6/P7 as outputs (1)
    static constexpr uint8_t CONFIG_MASK = MASK_SX_LNA_EN | MASK_SX_ANT_SW | MASK_SX_NRST;

    bool readRegister(uint8_t reg, uint8_t& value);
    bool writeRegister(uint8_t reg, uint8_t value);

  public:
    GPIOExpanderManager();

    bool init();
    bool reinit(); // Re-initialize I2C bus

    // User Button
    ButtonEvent getButtonEvent(); // Returns BUTTON_EVENT_NONE, BUTTON_EVENT_SHORT_PRESS, or BUTTON_EVENT_LONG_PRESS

    // SX1262 Control Pins
    void setSXLNAEnabled(bool enabled);      // SX_LNA_EN (antenna amplifier)
    void setSXAntennaSwitched(bool enabled); // SX_ANT_SW (RF antenna switch)
    void setSXReset(bool state);             // SX_NRST (SX1262 reset)
};

extern GPIOExpanderManager gpioExpanderManager;

#endif // GPIO_EXPANDER_MANAGER_H
