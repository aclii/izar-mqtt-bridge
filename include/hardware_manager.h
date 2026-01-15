#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "gpio_expander_manager.h" // For ButtonEvent enum

#define NUM_LEDS 1 // Unit-C6L has single WS2812C LED

class HardwareManager {
  private:
    Adafruit_NeoPixel strip; // NeoPixel strip for WS2812C LED
    uint32_t rgbColor = 0x000000;
    SPIClass* sharedSPI = nullptr; // Shared SPI bus for display and LoRa

  public:
    HardwareManager();

    bool init();

    // Shared SPI bus access
    SPIClass* getSharedSPI() { return sharedSPI; }

    // Buzzer control
    void beep(int duration = 100, int frequency = 1000);
    void beepSuccess();

    // RGB LED control (WS2812C)
    void setRGBColor(uint8_t r, uint8_t g, uint8_t b);
    void setRGBColor(uint32_t color);
    void rgbOff();

    // Button
    ButtonEvent getButtonEvent(); // Returns NONE, SHORT_PRESS, or LONG_PRESS
};

extern HardwareManager hardwareManager;

#endif // HARDWARE_MANAGER_H
