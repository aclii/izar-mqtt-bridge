#include "hardware_manager.h"
#include "gpio_expander_manager.h"

HardwareManager hardwareManager;

HardwareManager::HardwareManager() : strip(NUM_LEDS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800) {}

bool HardwareManager::init() {
    // Initialize shared SPI bus for display and FSK modem
    LOG_DEBUG("Hardware", "Initializing shared SPI bus...");
    sharedSPI = new SPIClass(SPI);
    sharedSPI->begin(SX1262_SSD1306_SCK_PIN, SX1262_SSD1306_MISO_PIN, SX1262_SSD1306_MOSI_PIN, -1);
    sharedSPI->setFrequency(SX1262_SSD1306_SPI_FREQUENCY);
    sharedSPI->setDataMode(SPI_MODE0);
    sharedSPI->setBitOrder(MSBFIRST);
    LOG_DEBUG("Hardware", "Shared SPI initialized: SCK=%d, MOSI=%d, MISO=%d, Freq=%d MHz", SX1262_SSD1306_SCK_PIN,
              SX1262_SSD1306_MOSI_PIN, SX1262_SSD1306_MISO_PIN, SX1262_SSD1306_SPI_FREQUENCY / 1000000);

    // Initialize buzzer
    if (ENABLE_BUZZER) {
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        LOG_DEBUG("Hardware", "Buzzer initialized on GPIO %d", BUZZER_PIN);
    }

    // Initialize RGB LED (WS2812C NeoPixel)
    if (ENABLE_RGB_LED) {
        strip.begin(); // Initialize NeoPixel strip
        strip.show();  // Turn all pixels off
        LOG_DEBUG("Hardware", "RGB LED (WS2812C) initialized on GPIO %d", RGB_LED_PIN);
    }

    // Test sound and LED
    beepSuccess();
    setRGBColor(0, 255, 0); // Green
    delay(200);
    rgbOff();

    LOG_INFO("Hardware", "Manager initialized successfully");
    return true;
}

void HardwareManager::beep(int duration, int frequency) {
    if (!ENABLE_BUZZER)
        return;

    tone(BUZZER_PIN, frequency, duration);
}

void HardwareManager::beepSuccess() {
    beep(100, 1000);
    delay(100);
    beep(100, 1200);
    delay(100);
    beep(100, 1400);
}

void HardwareManager::setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = strip.Color(r, g, b);
    setRGBColor(color);
}

void HardwareManager::setRGBColor(uint32_t color) {
    if (!ENABLE_RGB_LED)
        return;

    rgbColor = color;

    // Set pixel color and update
    strip.setPixelColor(0, color);
    strip.show();
}

void HardwareManager::rgbOff() {
    if (!ENABLE_RGB_LED)
        return;

    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
}

ButtonEvent HardwareManager::getButtonEvent() {
    return gpioExpanderManager.getButtonEvent();
}
