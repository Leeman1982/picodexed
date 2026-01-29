/*
 * PicoDexed RP2350 - Arduino Port
 * Voice Manager Implementation
 *
 * Loads DX7 voice banks from LittleFS (.syx files) or falls back
 * to the built-in default voice.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#include "voice_manager.h"
#include "synth_engine.h"
#include "oled_display.h"

#if VOICE_STORAGE_ENABLED
#include <LittleFS.h>
#endif

// ============================================================================
// Default voice bank: Brass 1 (first voice, rest zeroed)
// This ensures sound works even without any .syx files loaded.
// ============================================================================
static const uint8_t DEFAULT_VOICE_SYX[VOICE_SYX_SIZE] PROGMEM = {
    // Brass 1 in DX7 packed SysEx format (128 bytes)
    0x62, 0x63, 0x1C, 0x44, 0x62, 0x62, 0x5B, 0x00,
    0x27, 0x36, 0x32, 0x01, 0x01, 0x04, 0x00, 0x02,
    0x52, 0x00, 0x01, 0x00, 0x07, 0x4D, 0x24, 0x29,
    0x47, 0x63, 0x62, 0x62, 0x00, 0x27, 0x00, 0x00,
    0x03, 0x03, 0x00, 0x00, 0x02, 0x62, 0x00, 0x01,
    0x00, 0x08, 0x4D, 0x24, 0x29, 0x47, 0x63, 0x62,
    0x62, 0x00, 0x27, 0x00, 0x00, 0x03, 0x03, 0x00,
    0x00, 0x02, 0x63, 0x00, 0x01, 0x00, 0x07, 0x4D,
    0x4C, 0x52, 0x47, 0x63, 0x62, 0x62, 0x00, 0x27,
    0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x02, 0x63,
    0x00, 0x01, 0x00, 0x05, 0x3E, 0x33, 0x1D, 0x47,
    0x52, 0x5F, 0x60, 0x00, 0x1B, 0x00, 0x07, 0x03,
    0x01, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00,
    0x0E, 0x48, 0x4C, 0x63, 0x47, 0x63, 0x58, 0x60,
    0x15, 0x07, 0x01, 0x25, 0x00, 0x05, 0x00, 0x00,
    0x42, 0x52, 0x41, 0x53, 0x53, 0x20, 0x20, 0x20,
};

VoiceManager::VoiceManager()
    : synth(nullptr),
      oled(nullptr),
      numBanks(0),
      currentBank(0),
      currentVoice(0),
      bankSelPending(false)
{
    memset(bankData, 0, sizeof(bankData));
    memset(bankNames, 0, sizeof(bankNames));
}

bool VoiceManager::begin(SynthEngine *engine, OLEDDisplay *display) {
    synth = engine;
    oled  = display;

#if VOICE_STORAGE_ENABLED
    // Mount LittleFS
    if (!LittleFS.begin()) {
        DEBUG_PRINTLN("LittleFS mount failed, formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            DEBUG_PRINTLN("LittleFS format+mount failed");
        }
    }
    rescanBanks();
#endif

    // If no banks loaded from LittleFS, create a default bank
    if (numBanks == 0) {
        DEBUG_PRINTLN("No .syx files found, using default voice");
        numBanks = 1;
        strncpy(bankNames[0], "Default", sizeof(bankNames[0]));
        // Fill first voice with default, rest stays zeroed
        memcpy(bankData[0][0], DEFAULT_VOICE_SYX, VOICE_SYX_SIZE);
        // Copy default to all 32 slots so any program change works
        for (int i = 1; i < NUM_VOICES_PER_BANK; i++) {
            memcpy(bankData[0][i], DEFAULT_VOICE_SYX, VOICE_SYX_SIZE);
        }
    }

    DEBUG_PRINT("Voice manager: %d banks loaded\n", numBanks);

    // Load first voice
    currentBank = 0;
    currentVoice = 0;
    loadCurrentVoice();
    updateDisplay();

    return true;
}

void VoiceManager::rescanBanks() {
#if VOICE_STORAGE_ENABLED
    numBanks = 0;

    // Scan for .syx files in the voices directory
    File dir = LittleFS.open(VOICE_STORAGE_PATH);
    if (!dir || !dir.isDirectory()) {
        DEBUG_PRINTLN("No /voices directory on LittleFS");
        // Try creating it for future use
        LittleFS.mkdir(VOICE_STORAGE_PATH);
        return;
    }

    File file = dir.openNextFile();
    while (file && numBanks < MAX_BANKS) {
        String fname = file.name();
        if (fname.endsWith(".syx") || fname.endsWith(".SYX")) {
            String fullPath = String(VOICE_STORAGE_PATH) + "/" + fname;
            if (loadSyxFile(fullPath.c_str(), numBanks)) {
                // Extract bank name from filename (strip .syx)
                String name = fname.substring(0, fname.length() - 4);
                if (name.length() > 15) name = name.substring(0, 15);
                strncpy(bankNames[numBanks], name.c_str(), sizeof(bankNames[0]));
                numBanks++;
                DEBUG_PRINT("Loaded bank: %s\n", fname.c_str());
            }
        }
        file = dir.openNextFile();
    }
    dir.close();

    DEBUG_PRINT("Scanned LittleFS: %d banks found\n", numBanks);
#endif
}

bool VoiceManager::loadSyxFile(const char *path, uint8_t bankIdx) {
#if VOICE_STORAGE_ENABLED
    File file = LittleFS.open(path, "r");
    if (!file) return false;

    size_t fileSize = file.size();

    // DX7 bank SysEx: 4104 bytes (F0 43 00 09 20 00 ... F7)
    // Raw bank data: 4096 bytes (32 voices x 128 bytes)
    // Single voice SysEx: 163 bytes
    // Raw voice: 128 bytes

    if (fileSize == BANK_SYSEX_DUMP_SIZE) {
        // Full SysEx bank dump - skip 6-byte header
        file.seek(6);
        for (int v = 0; v < NUM_VOICES_PER_BANK; v++) {
            file.read(bankData[bankIdx][v], VOICE_SYX_SIZE);
        }
        file.close();
        return true;
    }
    else if (fileSize == BANK_SYX_SIZE) {
        // Raw bank data (32 x 128 bytes)
        for (int v = 0; v < NUM_VOICES_PER_BANK; v++) {
            file.read(bankData[bankIdx][v], VOICE_SYX_SIZE);
        }
        file.close();
        return true;
    }
    else if (fileSize >= VOICE_SYX_SIZE) {
        // Try reading as many voices as possible
        int numVoicesInFile = fileSize / VOICE_SYX_SIZE;
        if (numVoicesInFile > NUM_VOICES_PER_BANK)
            numVoicesInFile = NUM_VOICES_PER_BANK;

        for (int v = 0; v < numVoicesInFile; v++) {
            file.read(bankData[bankIdx][v], VOICE_SYX_SIZE);
        }
        // Fill remaining with first voice
        for (int v = numVoicesInFile; v < NUM_VOICES_PER_BANK; v++) {
            memcpy(bankData[bankIdx][v], bankData[bankIdx][0], VOICE_SYX_SIZE);
        }
        file.close();
        return true;
    }

    file.close();
    return false;
#else
    return false;
#endif
}

void VoiceManager::programChange(uint8_t program) {
    if (bankSelPending) {
        bankSelPending = false;
        if (program < NUM_VOICES_PER_BANK) {
            currentVoice = program;
            loadCurrentVoice();
            updateDisplay();
        }
    } else {
        // Map program 0-127 across banks
        uint8_t bank = program / NUM_VOICES_PER_BANK;
        uint8_t voice = program % NUM_VOICES_PER_BANK;
        if (bank < numBanks) {
            currentBank = bank;
            currentVoice = voice;
            loadCurrentVoice();
            updateDisplay();
        }
    }
}

void VoiceManager::bankSelectMSB(uint8_t msb) {
    // Only LSB is significant for DX7 (same as original)
    (void)msb;
}

void VoiceManager::bankSelectLSB(uint8_t lsb) {
    lsb = lsb % numBanks;
    currentBank = lsb;
    bankSelPending = true;
}

void VoiceManager::nextVoice() {
    int next = currentBank * NUM_VOICES_PER_BANK + currentVoice + 1;
    if (next >= numBanks * NUM_VOICES_PER_BANK) next = 0;
    currentBank  = (next / NUM_VOICES_PER_BANK) % numBanks;
    currentVoice = next % NUM_VOICES_PER_BANK;
    bankSelPending = false;
    loadCurrentVoice();
    updateDisplay();
}

void VoiceManager::prevVoice() {
    int next = currentBank * NUM_VOICES_PER_BANK + currentVoice - 1;
    if (next < 0) next = numBanks * NUM_VOICES_PER_BANK - 1;
    currentBank  = (next / NUM_VOICES_PER_BANK) % numBanks;
    currentVoice = next % NUM_VOICES_PER_BANK;
    bankSelPending = false;
    loadCurrentVoice();
    updateDisplay();
}

void VoiceManager::loadCurrentVoice() {
    if (synth && currentBank < numBanks && currentVoice < NUM_VOICES_PER_BANK) {
        synth->loadVoice(bankData[currentBank][currentVoice]);
    }
}

void VoiceManager::getCurrentVoiceName(char *name) {
    if (synth) {
        synth->getVoiceName(name);
    } else {
        strcpy(name, "----------");
    }
}

void VoiceManager::updateDisplay() {
    if (!oled) return;

    char voiceName[VOICE_NAME_SIZE + 1];
    getCurrentVoiceName(voiceName);
    oled->showVoice(currentBank, currentVoice, voiceName);
}
