/*
 * PicoDexed RP2350 - Arduino Port
 * Synth Engine Implementation
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#include "synth_engine.h"

// Constrain macro (same as Arduino constrain but safe from double-eval)
#define CLAMP(val, lo, hi) ((val) < (lo) ? (lo) : ((val) > (hi) ? (hi) : (val)))

SynthEngine::SynthEngine()
    : dexed(nullptr)
{
    mutex_init(&synthMutex);
    memset(voiceParams, 0, sizeof(voiceParams));
}

bool SynthEngine::begin() {
    dexed = new Dexed(POLYPHONY, SAMPLE_RATE);
    if (!dexed) return false;

    initControllers();

    // Load default voice (Brass 1) so there's sound immediately
    static const uint8_t defaultVoice[VOICE_SIZE] = {
         49,  99,  28,  68,  98,  98,  91,   0,
         39,  54,  50,   1,   1,   4,   0,   2,
         82,   0,   1,   0,   7,  77,  36,  41,
         71,  99,  98,  98,   0,  39,   0,   0,
          3,   3,   0,   0,   2,  98,   0,   1,
          0,   8,  77,  36,  41,  71,  99,  98,
         98,   0,  39,   0,   0,   3,   3,   0,
          0,   2,  99,   0,   1,   0,   7,  77,
         76,  82,  71,  99,  98,  98,   0,  39,
          0,   0,   3,   3,   0,   0,   2,  99,
          0,   1,   0,   5,  62,  51,  29,  71,
         82,  95,  96,   0,  27,   0,   7,   3,
          1,   0,   0,   0,  86,   0,   0,   0,
         14,  72,  76,  99,  71,  99,  88,  96,
          0,  39,   0,  14,   3,   3,   0,   0,
          0,  98,   0,   0,   0,  14,  84,  95,
         95,  60,  50,  50,  50,  50,  21,   7,
          1,  37,   0,   5,   0,   0,   4,   3,
         24,  66,  82,  65,  83,  83,  32,  32,
         32,  49,  32,  63
    };

    memcpy(voiceParams, defaultVoice, VOICE_SIZE);
    dexed->loadVoiceParameters(voiceParams);

    DEBUG_PRINT("Synth engine initialized: %d voices @ %d Hz\n", POLYPHONY, SAMPLE_RATE);
    return true;
}

void SynthEngine::initControllers() {
    dexed->setGain(1.0f);
    dexed->setPBController(2, 0);
    dexed->setMWController(99, 1, 0);
    dexed->setFCController(99, 1, 0);
    dexed->setBCController(99, 1, 0);
    dexed->setATController(99, 1, 0);
    dexed->ControllersRefresh();
}

// --- Audio generation (Core 1) ---

void SynthEngine::getSamples(int16_t *buffer, uint16_t numSamples) {
    mutex_enter_blocking(&synthMutex);
    // Use the float path and convert - RP2350 FPU makes this fast
    float tmpBuf[numSamples];
    dexed->getSamples(tmpBuf, numSamples);
    mutex_exit(&synthMutex);

    // Convert float to int16 using FPU
    for (uint16_t i = 0; i < numSamples; i++) {
        float val = tmpBuf[i] * 32767.0f;
        if (val > 32767.0f) val = 32767.0f;
        if (val < -32768.0f) val = -32768.0f;
        buffer[i] = (int16_t)val;
    }
}

// --- Note events (Core 0) ---

void SynthEngine::keyDown(uint8_t pitch, uint8_t velocity) {
    mutex_enter_blocking(&synthMutex);
    dexed->keydown(pitch, velocity);
    mutex_exit(&synthMutex);
}

void SynthEngine::keyUp(uint8_t pitch) {
    mutex_enter_blocking(&synthMutex);
    dexed->keyup(pitch);
    mutex_exit(&synthMutex);
}

void SynthEngine::panic() {
    mutex_enter_blocking(&synthMutex);
    dexed->panic();
    mutex_exit(&synthMutex);
}

void SynthEngine::notesOff() {
    mutex_enter_blocking(&synthMutex);
    dexed->notesOff();
    mutex_exit(&synthMutex);
}

// --- Voice loading (Core 0) ---

void SynthEngine::loadVoice(const uint8_t *sysexVoice) {
    mutex_enter_blocking(&synthMutex);
    dexed->decodeVoice(voiceParams, sysexVoice);
    dexed->loadVoiceParameters(voiceParams);
    mutex_exit(&synthMutex);
}

void SynthEngine::decodeAndLoadVoice(const uint8_t *packedVoice) {
    mutex_enter_blocking(&synthMutex);
    dexed->decodeVoice(voiceParams, packedVoice);
    dexed->loadVoiceParameters(voiceParams);
    mutex_exit(&synthMutex);
}

// --- Controllers (Core 0) ---

void SynthEngine::setVolume(uint8_t vol) {
    if (vol < 128) {
        mutex_enter_blocking(&synthMutex);
        float gain = (float)vol / 127.0f;
        dexed->setGain(gain);
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setAftertouch(uint8_t value) {
    if (value < 128) {
        mutex_enter_blocking(&synthMutex);
        dexed->setAftertouch(value);
        dexed->ControllersRefresh();
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setModWheel(uint8_t value) {
    if (value < 128) {
        mutex_enter_blocking(&synthMutex);
        dexed->setModWheel(value);
        dexed->ControllersRefresh();
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setBreathController(uint8_t value) {
    if (value < 128) {
        mutex_enter_blocking(&synthMutex);
        dexed->setBreathController(value);
        dexed->ControllersRefresh();
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setFootController(uint8_t value) {
    if (value < 128) {
        mutex_enter_blocking(&synthMutex);
        dexed->setFootController(value);
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setSustain(bool on) {
    mutex_enter_blocking(&synthMutex);
    dexed->setSustain(on);
    mutex_exit(&synthMutex);
}

void SynthEngine::setPortamento(uint8_t value) {
    mutex_enter_blocking(&synthMutex);
    if (value < 64) {
        dexed->setPortamento(0, 0, 0);
    } else if (value < 127) {
        dexed->setPortamento(1, 1, 60);
    }
    mutex_exit(&synthMutex);
}

void SynthEngine::setMasterTune(uint8_t value) {
    if (value < 128) {
        mutex_enter_blocking(&synthMutex);
        dexed->setMasterTune(value);
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setPitchBend(uint8_t lsb, uint8_t msb) {
    if (lsb < 128 && msb < 128) {
        mutex_enter_blocking(&synthMutex);
        dexed->setPitchbend(lsb, msb);
        mutex_exit(&synthMutex);
    }
}

void SynthEngine::setMonoMode(bool mono) {
    mutex_enter_blocking(&synthMutex);
    dexed->setMonoMode(mono);
    mutex_exit(&synthMutex);
}

// --- Pitch bend / portamento config ---

void SynthEngine::setPitchbendRange(uint8_t range) {
    mutex_enter_blocking(&synthMutex);
    dexed->setPitchbendRange(CLAMP(range, 0, 12));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setPitchbendStep(uint8_t step) {
    mutex_enter_blocking(&synthMutex);
    dexed->setPitchbendStep(CLAMP(step, 0, 12));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setPortamentoMode(uint8_t mode) {
    mutex_enter_blocking(&synthMutex);
    dexed->setPortamentoMode(CLAMP(mode, 0, 1));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setPortamentoGlissando(uint8_t glissando) {
    mutex_enter_blocking(&synthMutex);
    dexed->setPortamentoGlissando(CLAMP(glissando, 0, 1));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setPortamentoTime(uint8_t time) {
    mutex_enter_blocking(&synthMutex);
    dexed->setPortamentoTime(CLAMP(time, 0, 99));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

// --- Controller ranges and targets ---

void SynthEngine::setModWheelRange(uint8_t range) {
    mutex_enter_blocking(&synthMutex);
    dexed->setMWController(range, dexed->getModWheelTarget(), 0);
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setModWheelTarget(uint8_t target) {
    mutex_enter_blocking(&synthMutex);
    dexed->setModWheelTarget(CLAMP(target, 0, 7));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setFootControllerRange(uint8_t range) {
    mutex_enter_blocking(&synthMutex);
    dexed->setFCController(range, dexed->getFootControllerTarget(), 0);
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setFootControllerTarget(uint8_t target) {
    mutex_enter_blocking(&synthMutex);
    dexed->setFootControllerTarget(CLAMP(target, 0, 7));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setBreathControllerRange(uint8_t range) {
    mutex_enter_blocking(&synthMutex);
    dexed->setBCController(range, dexed->getBreathControllerTarget(), 0);
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setBreathControllerTarget(uint8_t target) {
    mutex_enter_blocking(&synthMutex);
    dexed->setBreathControllerTarget(CLAMP(target, 0, 7));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setAftertouchRange(uint8_t range) {
    mutex_enter_blocking(&synthMutex);
    dexed->setATController(range, dexed->getAftertouchTarget(), 0);
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

void SynthEngine::setAftertouchTarget(uint8_t target) {
    mutex_enter_blocking(&synthMutex);
    dexed->setAftertouchTarget(CLAMP(target, 0, 7));
    dexed->ControllersRefresh();
    mutex_exit(&synthMutex);
}

// --- SysEx ---

int16_t SynthEngine::checkSystemExclusive(const uint8_t *msg, size_t len) {
    mutex_enter_blocking(&synthMutex);
    int16_t result = dexed->checkSystemExclusive(msg, len);
    mutex_exit(&synthMutex);
    return result;
}

void SynthEngine::setVoiceDataElement(uint8_t data, uint8_t number) {
    mutex_enter_blocking(&synthMutex);
    dexed->setVoiceDataElement(CLAMP(data, 0, 155), CLAMP(number, 0, 99));
    mutex_exit(&synthMutex);
}

void SynthEngine::loadVoiceParameters(const uint8_t *data) {
    uint8_t voice[161];
    memcpy(voice, data, sizeof(uint8_t) * 161);

    // Filter invalid characters in voice name
    for (uint8_t i = 0; i < 10; i++) {
        if (voice[151 + i] > 126)
            voice[151 + i] = 32;
    }

    mutex_enter_blocking(&synthMutex);
    dexed->loadVoiceParameters(&voice[6]);
    dexed->doRefreshVoice();
    mutex_exit(&synthMutex);
}

// --- Voice parameter access ---

void SynthEngine::getVoiceName(char *name) {
    memcpy(name, &voiceParams[VOICE_NAME_OFFSET], VOICE_NAME_SIZE);
    name[VOICE_NAME_SIZE] = '\0';
}
