/*
 * PicoDexed RP2350 - Arduino Port
 * OLED Display Implementation
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#include "oled_display.h"

OLEDDisplay::OLEDDisplay()
    : display(nullptr), needsUpdate(false)
{
}

bool OLEDDisplay::begin() {
    // Select I2C bus based on config
#if DISPLAY_I2C_BUS == 1
    Wire1.setSDA(DISPLAY_SDA_PIN);
    Wire1.setSCL(DISPLAY_SCL_PIN);
    Wire1.begin();
    Wire1.setClock(400000);
    display = new Adafruit_SSD1306(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire1, -1);
#else
    Wire.setSDA(DISPLAY_SDA_PIN);
    Wire.setSCL(DISPLAY_SCL_PIN);
    Wire.begin();
    Wire.setClock(400000);
    display = new Adafruit_SSD1306(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, -1);
#endif

    if (!display->begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDR)) {
        DEBUG_PRINTLN("SSD1306 init failed");
        return false;
    }

    display->clearDisplay();
    display->display();
    DEBUG_PRINTLN("OLED display initialized");
    return true;
}

void OLEDDisplay::showLogo() {
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(8, 8);
    display->print("picoDexed");
    display->display();
}

void OLEDDisplay::showVoice(uint8_t bank, uint8_t voice, const char *name) {
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE);

    // Line 1: Bank:Voice number
    display->setCursor(0, 0);
    char numBuf[12];
    snprintf(numBuf, sizeof(numBuf), "%d:%2d", bank, voice);
    display->print(numBuf);

    // Line 2: Voice name
    display->setCursor(0, 16);
    display->print(name);

    needsUpdate = true;
}

void OLEDDisplay::showMessage(const char *line1, const char *line2) {
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE);

    if (line1) {
        display->setCursor(0, 0);
        display->print(line1);
    }
    if (line2) {
        display->setCursor(0, 16);
        display->print(line2);
    }

    needsUpdate = true;
}

void OLEDDisplay::update() {
    if (needsUpdate) {
        display->display();
        needsUpdate = false;
    }
}

void OLEDDisplay::clear() {
    display->clearDisplay();
    display->display();
    needsUpdate = false;
}
