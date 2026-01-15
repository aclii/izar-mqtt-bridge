#include "display_manager.h"

DisplayManager displayManager;

DisplayManager::DisplayManager() {}

bool DisplayManager::init(SPIClass* sharedSPI) {
    if (!sharedSPI) {
        LOG_ERROR("Display", "Error: Shared SPI not provided");
        return false;
    }

    spi = sharedSPI;
    LOG_INFO("Display", "Initializing display with M5GFX...");
    LOG_DEBUG("Display", "CS=%d, DC=%d, RST=%d", SSD1306_CS_PIN, SSD1306_DC_PIN, SSD1306_RST_PIN);

    // Initialize M5GFX with auto-detection
    if (!display.init()) {
        LOG_ERROR("Display", "M5GFX initialization failed");
        return false;
    }

    // Set rotation and clear
    display.setRotation(0);
    display.setBrightness(DISPLAY_BRIGHTNESS);
    display.fillScreen(TFT_BLACK);

    // Show welcome message
    display.setTextColor(TFT_WHITE);

    printAligned("IZAR RC868\nI W R4\nMQTT\nBRIDGE\nV " PROJECT_VERSION, HAlign::CENTER, VAlign::MIDDLE);

    delay(5000);
    LOG_INFO("Display", "Display initialized successfully");
    LOG_DEBUG("Display", "Resolution: %dx%d pixels", display.width(), display.height());

    return true;
}

void DisplayManager::clear() {
    acquireSPIBus();

    display.fillScreen(TFT_BLACK);

    releaseSPIBus();
}

void DisplayManager::sleep() {
    acquireSPIBus();
    display.setBrightness(0);
    display.sleep();
    releaseSPIBus();
}

void DisplayManager::wake() {
    acquireSPIBus();
    display.wakeup();
    display.setBrightness(DISPLAY_BRIGHTNESS);
    releaseSPIBus();
}

void DisplayManager::printAligned(const char* text, HAlign hAlign, VAlign vAlign, int lineSpacing) {
    if (!text || strlen(text) == 0)
        return;

    acquireSPIBus();

    // Get font metrics
    int32_t fontH = display.fontHeight();

    // Use default line spacing if not specified (2px gap between lines)
    if (lineSpacing < 0) {
        lineSpacing = 2;
    }

    // Split text into lines
    String textStr = String(text);
    std::vector<String> lines;
    int startIdx = 0;
    int newlineIdx;

    while ((newlineIdx = textStr.indexOf('\n', startIdx)) != -1) {
        lines.push_back(textStr.substring(startIdx, newlineIdx));
        startIdx = newlineIdx + 1;
    }
    // Add last line
    if (startIdx < textStr.length()) {
        lines.push_back(textStr.substring(startIdx));
    }

    if (lines.empty()) {
        releaseSPIBus();
        return;
    }

    // Limit line spacing if it would make text exceed display height
    if (lines.size() > 1) {
        int32_t maxSpacing = (display.height() - (lines.size() * fontH)) / (lines.size() - 1);
        if (lineSpacing > maxSpacing) {
            lineSpacing = maxSpacing;
        }
    }

    // Calculate total block height: fontHeight per line + gap between lines
    int32_t blockHeight = (lines.size() * fontH) + ((lines.size() - 1) * lineSpacing);

    // Calculate starting Y position based on vertical alignment
    int32_t startY = 0;

    if (vAlign == VAlign::MIDDLE) {
        // Center the entire block vertically
        startY = (display.height() - blockHeight) / 2;
    } else if (vAlign == VAlign::BOTTOM) {
        // Align block to bottom
        startY = display.height() - blockHeight;
    }
    // TOP: startY = 0 (default)

    // Draw each line
    for (int i = 0; i < lines.size(); i++) {
        if (lines[i].length() == 0)
            continue;

        // Calculate Y: each line is fontHeight + gap below previous line
        int32_t y = startY + (i * (fontH + lineSpacing));

        // Calculate X position based on horizontal alignment
        int32_t x = 2; // Default left margin

        if (hAlign == HAlign::CENTER) {
            x = display.width() / 2;
        } else if (hAlign == HAlign::RIGHT) {
            x = display.width() - 2;
        }

        // Set text datum based on alignment
        lgfx::v1::textdatum_t datum;

        if (hAlign == HAlign::LEFT) {
            datum = lgfx::v1::textdatum_t::top_left;
        } else if (hAlign == HAlign::CENTER) {
            datum = lgfx::v1::textdatum_t::top_center;
        } else {
            datum = lgfx::v1::textdatum_t::top_right;
        }

        // Set datum and draw
        display.setTextColor(TFT_WHITE);
        display.setTextDatum(datum);
        display.drawString(lines[i].c_str(), x, y);
    }

    // Reset to default
    display.setTextDatum(lgfx::v1::textdatum_t::top_left);

    releaseSPIBus();
}

void DisplayManager::drawTimeoutLine(int height) {
    if (height <= 0)
        return;

    acquireSPIBus();

    // Clamp height to display height
    if (height > display.height()) {
        height = display.height();
    }

    // Draw vertical line from bottom to top on left edge (x=0)
    // Line goes from (0, display.height() - 1) up to (0, display.height() - height)
    int startY = display.height() - 1;
    int endY = display.height() - height;

    display.drawLine(0, startY, 0, endY, TFT_WHITE);

    releaseSPIBus();
}

void DisplayManager::acquireSPIBus() {
    // For now, this is a placeholder for mutex/semaphore support
    // if other threads access the SPI bus simultaneously
    noInterrupts(); // Simple protection
}

void DisplayManager::releaseSPIBus() {
    // Release SPI bus
    interrupts(); // Re-enable interrupts
}
