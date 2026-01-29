/*
 * ============================================================================
 * PicoDexed RP2350 - DX7 FM Synthesizer for Raspberry Pi Pico 2
 * ============================================================================
 *
 * Arduino IDE port of picodexed, leveraging RP2350 advantages:
 *   - Cortex-M33 hardware FPU for fast float synthesis (6x faster than RP2040)
 *   - 520KB SRAM for 24-voice polyphony and large voice bank storage
 *   - 48 kHz sample rate (vs 24 kHz on RP2040) for higher audio quality
 *   - Dual-core: Core 1 for audio, Core 0 for MIDI/UI
 *   - LittleFS on 4MB flash for storing voice banks (no SD card needed)
 *   - USB MIDI via TinyUSB + Serial MIDI (DIN-5/TRS)
 *
 * ============================================================================
 * REQUIRED LIBRARIES (install via Library Manager or GitHub):
 * ============================================================================
 *   1. Arduino Audio Tools     - https://github.com/pschatzmann/arduino-audio-tools
 *   2. Adafruit SSD1306        - Adafruit SSD1306 (Library Manager)
 *   3. Adafruit GFX Library    - Adafruit GFX Library (Library Manager)
 *   4. Adafruit TinyUSB        - Adafruit TinyUSB Library (Library Manager)
 *   5. Synth_Dexed             - See setup_synth_dexed.sh or install manually
 *
 * ============================================================================
 * ARDUINO IDE SETTINGS (Tools menu):
 * ============================================================================
 *   Board:           Raspberry Pi Pico 2
 *   CPU Architecture: ARM Cortex-M33 (for hardware FPU)
 *   Flash Size:      4MB (Sketch: 1MB, FS: 3MB)
 *   USB Stack:       Adafruit TinyUSB
 *   CPU Speed:       150 MHz (overclock handled in code)
 *
 * ============================================================================
 * HARDWARE CONNECTIONS (same pinout as original picodexed):
 * ============================================================================
 *   I2S DAC (e.g. PCM5102A):
 *     GPIO 9  -> DIN (data)
 *     GPIO 10 -> BCLK (bit clock)
 *     GPIO 11 -> LRCLK (word select)
 *
 *   MIDI Input (DIN-5 or TRS):
 *     GPIO 5  -> MIDI RX (via optocoupler)
 *     GPIO 4  -> MIDI TX (optional)
 *
 *   SSD1306 OLED (I2C):
 *     GPIO 2  -> SDA
 *     GPIO 3  -> SCL
 *
 *   Rotary Encoder:
 *     GPIO 6  -> A
 *     GPIO 7  -> B
 *     GPIO 8  -> Switch (optional)
 *
 * ============================================================================
 * VOICE BANK STORAGE (LittleFS - no SD card needed):
 * ============================================================================
 *   Voice banks are stored as .syx files on the onboard flash filesystem.
 *   The 4MB flash is split: 1MB for sketch code, 3MB for voice storage.
 *   That's room for ~750 voice banks (24,000+ voices).
 *
 *   To upload voice banks:
 *     1. Install the LittleFS upload plugin for Arduino IDE
 *     2. Place .syx files in a "data/voices/" folder in your sketch directory
 *     3. Use Tools -> "Pico LittleFS Data Upload"
 *
 *   Supported .syx formats:
 *     - DX7 bank dump (4104 bytes: 32 voices per file)
 *     - Raw bank data (4096 bytes: 32 x 128)
 *     - Individual voice files (128+ bytes)
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * https://github.com/diyelectromusic/picodexed
 *
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */

#include "config.h"
#include "AudioTools.h"
#include "synth_engine.h"
#include "midi_input.h"
#include "oled_display.h"
#include "voice_manager.h"

// ============================================================================
// Global objects
// ============================================================================
SynthEngine  synthEngine;
MIDIInput    midiInput;
OLEDDisplay  oledDisplay;
VoiceManager voiceManager;

// Audio output via I2S (Arduino Audio Tools)
I2SStream    i2sOutput;

// Audio buffer for Core 1
static int16_t audioBuffer[AUDIO_BUFFER_SIZE];

// Rotary encoder state
static volatile int  encoderPos     = 0;
static volatile int  lastEncoderPos = 0;
static volatile bool encoderALast   = false;
static uint32_t      lastEncoderTime = 0;

// ============================================================================
// Rotary Encoder ISR (GPIO interrupt - no PIO needed)
// ============================================================================
void encoderISR() {
    bool a = digitalRead(ENCODER_A_PIN);
    bool b = digitalRead(ENCODER_B_PIN);

    if (a != encoderALast) {
        if (b != a) {
            encoderPos++;
        } else {
            encoderPos--;
        }
        encoderALast = a;
    }
}

// ============================================================================
// CORE 0: Setup - MIDI, Display, Encoder, Voice Manager
// ============================================================================
void setup() {
    // Overclock RP2350 to 250 MHz (safe, stable)
#if OVERCLOCK_MHZ > 0
    set_sys_clock_khz(OVERCLOCK_MHZ * 1000, false);
#endif

    // USB Serial for debug output
    Serial.begin(115200);
    delay(500);

    DEBUG_PRINT("\n\nPicoDexed %s (RP2350 Arduino)\n", PICODEXED_VERSION);
    DEBUG_PRINT("Configuration:\n");
    DEBUG_PRINT("  Sample Rate:  %d Hz\n", SAMPLE_RATE);
    DEBUG_PRINT("  Polyphony:    %d voices\n", POLYPHONY);
    DEBUG_PRINT("  CPU Clock:    %d MHz\n", OVERCLOCK_MHZ > 0 ? OVERCLOCK_MHZ : 150);
    DEBUG_PRINT("  FPU:          Hardware (Cortex-M33)\n");
    DEBUG_PRINT("  SRAM:         520 KB\n");
    DEBUG_PRINTLN("---");

    // LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);

    // Initialize OLED display
    oledDisplay.begin();
    oledDisplay.showLogo();
    delay(1500);

    // Initialize synth engine
    if (!synthEngine.begin()) {
        DEBUG_PRINTLN("ERROR: Synth engine init failed!");
        oledDisplay.showMessage("SYNTH", "ERROR!");
        while (1) { delay(1000); }
    }

    // Initialize voice manager (loads banks from LittleFS)
    voiceManager.begin(&synthEngine, &oledDisplay);

    // Initialize MIDI input (Serial + USB)
    midiInput.begin(&synthEngine);

    // Initialize rotary encoder with GPIO interrupts
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
    encoderALast = digitalRead(ENCODER_A_PIN);
    attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);

    DEBUG_PRINTLN("PicoDexed ready!");
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
}

// ============================================================================
// CORE 0: Loop - MIDI processing, encoder, display updates
// ============================================================================
void loop() {
    // Process MIDI input (Serial + USB)
    midiInput.process();

    // Check for MIDI program change / bank select
    // (handled inside midiInput.process() -> dispatchMessage)

    // Process rotary encoder
    uint32_t now = millis();
    if (now - lastEncoderTime >= ENCODER_DEBOUNCE_MS) {
        int pos = encoderPos;
        if (pos != lastEncoderPos) {
#if ENCODER_REVERSE
            if (pos < lastEncoderPos) {
#else
            if (pos > lastEncoderPos) {
#endif
                voiceManager.nextVoice();
            } else {
                voiceManager.prevVoice();
            }
            lastEncoderPos = pos;
            lastEncoderTime = now;
        }
    }

    // Update display if needed
    oledDisplay.update();
}

// ============================================================================
// CORE 1: Setup - I2S Audio Output
// ============================================================================
void setup1() {
    // Small delay to let Core 0 finish synth initialization
    delay(100);

    // Configure I2S output via Arduino Audio Tools
    auto config = i2sOutput.defaultConfig(TX_MODE);
    config.sample_rate    = SAMPLE_RATE;
    config.bits_per_sample = 16;
    config.pin_bck        = I2S_BCLK_PIN;
    config.pin_ws         = I2S_LRCLK_PIN;
    config.pin_data       = I2S_DATA_PIN;

#if STEREO_OUTPUT
    config.channels       = 2;  // Stereo (mono duplicated to both channels)
#else
    config.channels       = 1;  // True mono
#endif

    config.buffer_size    = AUDIO_BUFFER_SIZE * sizeof(int16_t);
    config.buffer_count   = 4;  // Multiple buffers for smooth playback

    if (!i2sOutput.begin(config)) {
        // I2S init failed - signal on LED
        while (1) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    }
}

// ============================================================================
// CORE 1: Loop - Continuous audio generation
// ============================================================================
void loop1() {
    // Generate audio samples from the Dexed synth engine
    synthEngine.getSamples(audioBuffer, AUDIO_BUFFER_SIZE);

#if STEREO_OUTPUT
    // For stereo output, duplicate mono to both channels
    int16_t stereoBuffer[AUDIO_BUFFER_SIZE * 2];
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        stereoBuffer[i * 2]     = audioBuffer[i];  // Left
        stereoBuffer[i * 2 + 1] = audioBuffer[i];  // Right
    }
    i2sOutput.write((uint8_t *)stereoBuffer, AUDIO_BUFFER_SIZE * 2 * sizeof(int16_t));
#else
    i2sOutput.write((uint8_t *)audioBuffer, AUDIO_BUFFER_SIZE * sizeof(int16_t));
#endif
}
