#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <M5GFX.h>
#include "config.h"

class DisplayManager {
  private:
    M5GFX display;
    SPIClass* spi = nullptr;
    unsigned long lastUpdate = 0;

    // SPI sharing - protect bus access
    void acquireSPIBus();
    void releaseSPIBus();

  public:
    DisplayManager();

    bool init(SPIClass* sharedSPI);

    void clear();
    void sleep(); // Turn off display
    void wake();  // Turn on display

    // Smart print with automatic alignment
    enum class HAlign { LEFT, CENTER, RIGHT };
    enum class VAlign { TOP, MIDDLE, BOTTOM };

    void printAligned(const char* text, HAlign hAlign = HAlign::LEFT, VAlign vAlign = VAlign::TOP,
                      int lineSpacing = -1);
    void drawTimeoutLine(int height); // Draw timeout indicator line (0-48 pixels)

  private:
};

extern DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
