/*
 * PicoDexed RP2350 - Arduino Port
 * Voice Manager - Bank/voice selection + LittleFS storage
 *
 * Voice data can come from two sources:
 *   1. Built-in default voice (Brass 1) compiled into flash
 *   2. .syx files stored on LittleFS (onboard flash filesystem)
 *
 * LittleFS Setup:
 *   - Set Flash Size to "4MB (Sketch: 1MB, FS: 3MB)" in Arduino IDE
 *   - Upload .syx files to /voices/ using the LittleFS upload tool
 *   - Files are auto-discovered and loaded as voice banks
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef VOICE_MANAGER_H
#define VOICE_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Forward declarations
class SynthEngine;
class OLEDDisplay;

// A voice bank is 32 voices x 128 bytes = 4096 bytes (DX7 SysEx format)
#define BANK_SYX_SIZE       (NUM_VOICES_PER_BANK * VOICE_SYX_SIZE)
// SysEx bulk dump has 6 header bytes + 4096 data + 2 footer bytes = 4104
#define BANK_SYSEX_DUMP_SIZE 4104

class VoiceManager {
public:
    VoiceManager();

    // Initialize: scan LittleFS for .syx banks, or use defaults
    bool begin(SynthEngine *engine, OLEDDisplay *display);

    // Voice selection
    void programChange(uint8_t program);
    void bankSelectMSB(uint8_t msb);
    void bankSelectLSB(uint8_t lsb);

    // Encoder navigation (next/prev voice across all banks)
    void nextVoice();
    void prevVoice();

    // Current state
    uint8_t getCurrentBank() const { return currentBank; }
    uint8_t getCurrentVoice() const { return currentVoice; }
    uint8_t getNumBanks() const { return numBanks; }
    void getCurrentVoiceName(char *name);

    // Reload banks from LittleFS
    void rescanBanks();

private:
    void loadCurrentVoice();
    void updateDisplay();
    bool loadSyxFile(const char *path, uint8_t bankIdx);

    SynthEngine *synth;
    OLEDDisplay *oled;

    // Voice bank storage (in RAM for fast access)
    // On RP2350 with 520KB SRAM, we can hold many banks
    // 8 banks x 32 voices x 128 bytes = 32KB - fits easily
    static const uint8_t MAX_BANKS = 32;
    uint8_t bankData[MAX_BANKS][NUM_VOICES_PER_BANK][VOICE_SYX_SIZE];
    char    bankNames[MAX_BANKS][16];

    uint8_t numBanks;
    uint8_t currentBank;
    uint8_t currentVoice;
    bool    bankSelPending;
};

#endif // VOICE_MANAGER_H
