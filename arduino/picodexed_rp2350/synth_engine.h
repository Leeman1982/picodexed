/*
 * PicoDexed RP2350 - Arduino Port
 * Synth Engine - Thread-safe Dexed wrapper for dual-core RP2350
 *
 * Core 0: MIDI events, voice loading, controller changes
 * Core 1: Audio sample generation
 * Uses mutex for thread safety between cores.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef SYNTH_ENGINE_H
#define SYNTH_ENGINE_H

#include <Arduino.h>
#include <mutex>
#include "config.h"

// Forward declare the Dexed class from Synth_Dexed library
#include "synth_dexed.h"

class SynthEngine {
public:
    SynthEngine();

    bool begin();

    // --- Audio generation (called from Core 1) ---
    // Fills buffer with 16-bit mono PCM samples
    void getSamples(int16_t *buffer, uint16_t numSamples);

    // --- Note events (called from Core 0) ---
    void keyDown(uint8_t pitch, uint8_t velocity);
    void keyUp(uint8_t pitch);
    void panic();
    void notesOff();

    // --- Voice loading (called from Core 0) ---
    void loadVoice(const uint8_t *sysexVoice);
    void decodeAndLoadVoice(const uint8_t *packedVoice);

    // --- Controllers (called from Core 0) ---
    void setVolume(uint8_t vol);
    void setAftertouch(uint8_t value);
    void setModWheel(uint8_t value);
    void setBreathController(uint8_t value);
    void setFootController(uint8_t value);
    void setSustain(bool on);
    void setPortamento(uint8_t value);
    void setMasterTune(uint8_t value);
    void setPitchBend(uint8_t lsb, uint8_t msb);
    void setMonoMode(bool mono);

    // --- Pitch bend / portamento config ---
    void setPitchbendRange(uint8_t range);
    void setPitchbendStep(uint8_t step);
    void setPortamentoMode(uint8_t mode);
    void setPortamentoGlissando(uint8_t glissando);
    void setPortamentoTime(uint8_t time);

    // --- Controller ranges and targets ---
    void setModWheelRange(uint8_t range);
    void setModWheelTarget(uint8_t target);
    void setFootControllerRange(uint8_t range);
    void setFootControllerTarget(uint8_t target);
    void setBreathControllerRange(uint8_t range);
    void setBreathControllerTarget(uint8_t target);
    void setAftertouchRange(uint8_t range);
    void setAftertouchTarget(uint8_t target);

    // --- SysEx ---
    int16_t checkSystemExclusive(const uint8_t *msg, size_t len);
    void setVoiceDataElement(uint8_t data, uint8_t number);
    void loadVoiceParameters(const uint8_t *data);

    // --- Voice parameter access ---
    void getVoiceName(char *name);  // Returns 10-char name from loaded voice

private:
    Dexed *dexed;
    uint8_t voiceParams[VOICE_SIZE];

    // Mutex for thread-safe access between Core 0 and Core 1
    // auto_init_mutex is provided by arduino-pico
    mutex_t synthMutex;

    void initControllers();
};

#endif // SYNTH_ENGINE_H
