/*
 * PicoDexed RP2350 - Arduino Port
 * MIDI Input Handler - Serial MIDI + USB MIDI
 *
 * Uses the FortySevenEffects MIDI Library for serial MIDI parsing,
 * and Adafruit TinyUSB for USB MIDI. Falls back to a built-in parser
 * if the MIDI library is not available.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef MIDI_INPUT_H
#define MIDI_INPUT_H

#include <Arduino.h>
#include "config.h"

// Forward declaration
class SynthEngine;

// ============================================================================
// SysEx constants
// ============================================================================
#define SYSEX_MAX_SIZE      1024
#define SYSEX_MANID_YAMAHA  0x43

// ============================================================================
// MIDI Message Types
// ============================================================================
enum MidiMsgType : uint8_t {
    MSG_INVALID           = 0x00,
    MSG_NOTE_OFF          = 0x80,
    MSG_NOTE_ON           = 0x90,
    MSG_AFTERTOUCH_POLY   = 0xA0,
    MSG_CONTROL_CHANGE    = 0xB0,
    MSG_PROGRAM_CHANGE    = 0xC0,
    MSG_AFTERTOUCH_CHAN   = 0xD0,
    MSG_PITCH_BEND        = 0xE0,
    MSG_SYSEX_START       = 0xF0,
    MSG_SYSEX_END         = 0xF7,
    MSG_CLOCK             = 0xF8,
    MSG_START             = 0xFA,
    MSG_CONTINUE          = 0xFB,
    MSG_STOP              = 0xFC,
    MSG_ACTIVE_SENSING    = 0xFE,
    MSG_SYSTEM_RESET      = 0xFF,
};

// ============================================================================
// Parsed MIDI message
// ============================================================================
struct MidiMessage {
    uint8_t  type;
    uint8_t  channel;   // 1-based
    uint8_t  data1;
    uint8_t  data2;
    uint8_t  sysex[SYSEX_MAX_SIZE];
    uint16_t sysexLen;
    bool     valid;
};

// ============================================================================
// MIDIInput class - handles both Serial and USB MIDI
// ============================================================================
class MIDIInput {
public:
    MIDIInput();

    void begin(SynthEngine *engine);
    void process();

    void setChannel(uint8_t ch);   // 1-16, or 0 for OMNI
    uint8_t getChannel() const;

private:
    // Serial MIDI
    void processSerialMIDI();
    bool readSerialByte(uint8_t *byte);
    void parseAndDispatch(uint8_t byte);

    // USB MIDI
    void processUSBMIDI();

    // Message dispatch
    void dispatchMessage(const MidiMessage &msg);
    void handleSysEx(const uint8_t *data, uint16_t len);

    SynthEngine *synth;
    uint8_t channel;        // Active MIDI channel (1-16, 0=OMNI)

    // Parser state
    MidiMessage currentMsg;
    uint8_t runningStatus;
    uint8_t pendingData[3];
    uint8_t pendingIdx;
    uint8_t pendingExpectedLen;
    bool    inSysEx;
    uint16_t sysexIdx;
};

#endif // MIDI_INPUT_H
