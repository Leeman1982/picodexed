/*
 * PicoDexed RP2350 - Arduino Port
 * OLED Display Manager - SSD1306 via Adafruit library
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

class OLEDDisplay {
public:
    OLEDDisplay();

    bool begin();
    void showLogo();
    void showVoice(uint8_t bank, uint8_t voice, const char *name);
    void showMessage(const char *line1, const char *line2 = nullptr);
    void update();
    void clear();

private:
    Adafruit_SSD1306 *display;
    bool needsUpdate;
};

#endif // OLED_DISPLAY_H
