/*
 * PicoDexed RP2350 - Arduino Port
 * DX7 FM Synthesizer for Raspberry Pi Pico 2 (RP2350)
 *
 * Configuration file - pin assignments, audio settings, features.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * https://github.com/diyelectromusic/picodexed
 *
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// VERSION
// ============================================================================
#define PICODEXED_VERSION "v1.00-rp2350"

// ============================================================================
// RP2350 PERFORMANCE SETTINGS
// ============================================================================
// The RP2350 Cortex-M33 runs at 150 MHz base, with hardware FPU.
// With overclocking to 250 MHz + FPU, we can run 24-voice polyphony at 48 kHz.
//
// Recommended configurations:
//   48000 Hz, 24 voices  (RP2350 @ 250 MHz - excellent quality)
//   48000 Hz, 16 voices  (RP2350 @ 150 MHz - good quality)
//   24000 Hz, 24 voices  (RP2350 @ 150 MHz - lower quality, max polyphony)
//
#define SAMPLE_RATE          48000
#define POLYPHONY            24
#define AUDIO_BUFFER_SIZE    256

// Overclock RP2350 to 250 MHz (safe, well within spec)
#define OVERCLOCK_MHZ        250

// ============================================================================
// AUDIO OUTPUT - I2S via Arduino Audio Tools
// ============================================================================
// Default pin assignment matches original picodexed wiring.
// I2S uses PIO under the hood (arduino-pico core handles this).
//
#define I2S_DATA_PIN         9
#define I2S_BCLK_PIN         10
#define I2S_LRCLK_PIN        11   // Must be BCLK + 1 for PIO I2S

// Set to 1 for stereo output (duplicates mono to both channels)
// Set to 0 for true mono (single channel I2S)
#define STEREO_OUTPUT        1

// ============================================================================
// MIDI CONFIGURATION
// ============================================================================
#define MIDI_CHANNEL         1     // 1-16, or 0 for OMNI
#define MIDI_SYSEX_DEVICE_ID 0     // Yamaha SysEx device number (0-15)

// Serial MIDI (DIN-5 / TRS via UART)
#define MIDI_SERIAL_PORT     Serial1   // Use Serial1 (UART1)
#define MIDI_BAUD_RATE       31250
#define MIDI_TX_PIN          4
#define MIDI_RX_PIN          5

// USB MIDI - enabled via Adafruit TinyUSB stack
// Select "Adafruit TinyUSB" as USB Stack in Arduino IDE Tools menu
#define USB_MIDI_ENABLED     1

// ============================================================================
// DISPLAY - SSD1306 OLED via I2C
// ============================================================================
#define DISPLAY_WIDTH        128
#define DISPLAY_HEIGHT       32
#define DISPLAY_I2C_ADDR     0x3C
#define DISPLAY_SDA_PIN      2
#define DISPLAY_SCL_PIN      3
#define DISPLAY_I2C_BUS      1     // 0 = Wire (I2C0), 1 = Wire1 (I2C1)

// ============================================================================
// ROTARY ENCODER
// ============================================================================
#define ENCODER_A_PIN        6
#define ENCODER_B_PIN        7
#define ENCODER_SW_PIN       8     // Push button (active low)
#define ENCODER_REVERSE      0     // Set to 1 to reverse direction

// Debounce interval in milliseconds
#define ENCODER_DEBOUNCE_MS  5

// ============================================================================
// VOICE BANKS
// ============================================================================
#define NUM_BANKS            8
#define NUM_VOICES_PER_BANK  32
#define VOICE_SYX_SIZE       128   // Packed SysEx voice size
#define VOICE_SIZE           156   // Unpacked voice parameter size
#define VOICE_NAME_SIZE      10
#define VOICE_NAME_OFFSET    (VOICE_SIZE - VOICE_NAME_SIZE - 1)

// ============================================================================
// LITTLEFS VOICE STORAGE
// ============================================================================
// Voice banks can be loaded from LittleFS on the onboard flash.
// Set Flash Size to "4MB (Sketch: 1MB, FS: 3MB)" in Arduino IDE.
// Upload .syx files to /voices/ directory using the LittleFS upload tool.
//
#define VOICE_STORAGE_ENABLED  1
#define VOICE_STORAGE_PATH     "/voices"

// ============================================================================
// LED (built-in)
// ============================================================================
// Pico 2 onboard LED is on GPIO 25
#define LED_PIN              LED_BUILTIN

// ============================================================================
// DEBUG
// ============================================================================
// Set to 1 to enable debug output on Serial (USB CDC)
#define DEBUG_ENABLED        1

#if DEBUG_ENABLED
  #define DEBUG_PRINT(...)   Serial.printf(__VA_ARGS__)
  #define DEBUG_PRINTLN(x)   Serial.println(x)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(x)
#endif

#endif // CONFIG_H
