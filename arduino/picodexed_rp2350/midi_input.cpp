/*
 * PicoDexed RP2350 - Arduino Port
 * MIDI Input Implementation
 *
 * Handles Serial MIDI (DIN-5/TRS @ 31250 baud) and USB MIDI.
 * The MIDI parser is ported from the original picodexed MIDIDevice class.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#include "midi_input.h"
#include "synth_engine.h"

#if USB_MIDI_ENABLED
#include <Adafruit_TinyUSB.h>
// USB MIDI object
static Adafruit_USBD_MIDI usbMidiDevice;
#endif

MIDIInput::MIDIInput()
    : synth(nullptr),
      channel(MIDI_CHANNEL),
      runningStatus(0),
      pendingIdx(0),
      pendingExpectedLen(0),
      inSysEx(false),
      sysexIdx(0)
{
    memset(&currentMsg, 0, sizeof(currentMsg));
    memset(pendingData, 0, sizeof(pendingData));
}

void MIDIInput::begin(SynthEngine *engine) {
    synth = engine;

    // Initialize Serial MIDI (UART)
    MIDI_SERIAL_PORT.setRX(MIDI_RX_PIN);
    MIDI_SERIAL_PORT.setTX(MIDI_TX_PIN);
    MIDI_SERIAL_PORT.begin(MIDI_BAUD_RATE);
    DEBUG_PRINT("Serial MIDI enabled on UART (RX=%d)\n", MIDI_RX_PIN);

#if USB_MIDI_ENABLED
    // Initialize USB MIDI via TinyUSB
    usbMidiDevice.begin();
    DEBUG_PRINTLN("USB MIDI enabled");
#endif
}

void MIDIInput::setChannel(uint8_t ch) {
    channel = ch;
}

uint8_t MIDIInput::getChannel() const {
    return channel;
}

void MIDIInput::process() {
    processSerialMIDI();
#if USB_MIDI_ENABLED
    processUSBMIDI();
#endif
}

// ============================================================================
// Serial MIDI Processing
// ============================================================================

void MIDIInput::processSerialMIDI() {
    while (MIDI_SERIAL_PORT.available()) {
        uint8_t byte = MIDI_SERIAL_PORT.read();
        parseAndDispatch(byte);
    }
}

// ============================================================================
// USB MIDI Processing
// ============================================================================

void MIDIInput::processUSBMIDI() {
#if USB_MIDI_ENABLED
    uint8_t packet[4];
    while (usbMidiDevice.available()) {
        usbMidiDevice.read(packet, sizeof(packet));

        // USB MIDI packet format: [CIN+Cable, Status, Data1, Data2]
        uint8_t cin    = packet[0] & 0x0F;
        uint8_t status = packet[1];
        uint8_t d1     = packet[2];
        uint8_t d2     = packet[3];

        // Parse based on Code Index Number
        MidiMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.valid = true;

        switch (cin) {
            case 0x8: // Note Off
                msg.type    = MSG_NOTE_OFF;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                msg.data2   = d2;
                break;
            case 0x9: // Note On
                msg.type    = MSG_NOTE_ON;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                msg.data2   = d2;
                break;
            case 0xA: // Poly Aftertouch
                msg.type    = MSG_AFTERTOUCH_POLY;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                msg.data2   = d2;
                break;
            case 0xB: // Control Change
                msg.type    = MSG_CONTROL_CHANGE;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                msg.data2   = d2;
                break;
            case 0xC: // Program Change
                msg.type    = MSG_PROGRAM_CHANGE;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                break;
            case 0xD: // Channel Aftertouch
                msg.type    = MSG_AFTERTOUCH_CHAN;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                break;
            case 0xE: // Pitch Bend
                msg.type    = MSG_PITCH_BEND;
                msg.channel = (status & 0x0F) + 1;
                msg.data1   = d1;
                msg.data2   = d2;
                break;
            case 0x4: // SysEx start/continue
            case 0x5: // SysEx end (1 byte)
            case 0x6: // SysEx end (2 bytes)
            case 0x7: // SysEx end (3 bytes)
                // Feed bytes into the serial parser for SysEx assembly
                for (int i = 1; i < 4; i++) {
                    if (packet[i] != 0) {
                        parseAndDispatch(packet[i]);
                    }
                }
                msg.valid = false;
                break;
            default:
                msg.valid = false;
                break;
        }

        if (msg.valid) {
            dispatchMessage(msg);
        }
    }
#endif
}

// ============================================================================
// MIDI Parser (ported from original picodexed CMIDIDevice)
// ============================================================================

void MIDIInput::parseAndDispatch(uint8_t byte) {
    // Handle SysEx accumulation
    if (inSysEx) {
        if (byte == MSG_SYSEX_END) {
            if (sysexIdx < SYSEX_MAX_SIZE) {
                currentMsg.sysex[sysexIdx++] = byte;
            }
            currentMsg.sysexLen = sysexIdx;
            currentMsg.type = MSG_SYSEX_START;
            currentMsg.valid = true;
            inSysEx = false;

            // Handle SysEx
            handleSysEx(currentMsg.sysex, currentMsg.sysexLen);
            return;
        }
        else if (byte >= 0x80) {
            // Real-time messages can interleave with SysEx
            if (byte >= 0xF8) {
                // Ignore real-time messages during SysEx
                return;
            }
            // Any other status byte aborts SysEx
            inSysEx = false;
            sysexIdx = 0;
            // Fall through to parse this byte as a new message
        }
        else {
            if (sysexIdx < SYSEX_MAX_SIZE) {
                currentMsg.sysex[sysexIdx++] = byte;
            }
            return;
        }
    }

    // Status byte?
    if (byte >= 0x80) {
        // System Real-Time (can appear anywhere)
        if (byte >= 0xF8) {
            return; // Ignore clock, active sensing, etc.
        }

        // SysEx Start
        if (byte == MSG_SYSEX_START) {
            inSysEx = true;
            sysexIdx = 0;
            currentMsg.sysex[sysexIdx++] = byte;
            return;
        }

        // Start new message
        pendingData[0] = byte;
        pendingIdx = 0;

        // Determine expected length
        switch (byte & 0xF0) {
            case MSG_NOTE_OFF:
            case MSG_NOTE_ON:
            case MSG_AFTERTOUCH_POLY:
            case MSG_CONTROL_CHANGE:
            case MSG_PITCH_BEND:
                pendingExpectedLen = 3;
                break;
            case MSG_PROGRAM_CHANGE:
            case MSG_AFTERTOUCH_CHAN:
                pendingExpectedLen = 2;
                break;
            default:
                pendingExpectedLen = 0;
                return;
        }

        // Update running status for channel messages
        if (byte < 0xF0) {
            runningStatus = byte;
        }

        pendingIdx = 1;
        return;
    }

    // Data byte
    if (pendingIdx == 0) {
        // Running status
        if (runningStatus >= 0x80 && runningStatus < 0xF0) {
            pendingData[0] = runningStatus;
            switch (runningStatus & 0xF0) {
                case MSG_NOTE_OFF:
                case MSG_NOTE_ON:
                case MSG_AFTERTOUCH_POLY:
                case MSG_CONTROL_CHANGE:
                case MSG_PITCH_BEND:
                    pendingExpectedLen = 3;
                    break;
                case MSG_PROGRAM_CHANGE:
                case MSG_AFTERTOUCH_CHAN:
                    pendingExpectedLen = 2;
                    break;
                default:
                    return;
            }
            pendingData[1] = byte;
            pendingIdx = 2;
        } else {
            return; // No running status, discard
        }
    } else {
        pendingData[pendingIdx] = byte;
        pendingIdx++;
    }

    // Check if message complete
    if (pendingIdx >= pendingExpectedLen) {
        MidiMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.type    = pendingData[0] & 0xF0;
        msg.channel = (pendingData[0] & 0x0F) + 1;
        msg.data1   = (pendingExpectedLen >= 2) ? pendingData[1] : 0;
        msg.data2   = (pendingExpectedLen >= 3) ? pendingData[2] : 0;
        msg.valid   = true;
        pendingIdx  = 0;

        dispatchMessage(msg);
    }
}

// ============================================================================
// Message Dispatch
// ============================================================================

void MIDIInput::dispatchMessage(const MidiMessage &msg) {
    if (!msg.valid || !synth) return;

    // Channel filter (0 = OMNI = accept all)
    if (channel != 0 && msg.channel != channel) {
        // SysEx is always processed regardless of channel
        if (msg.type != MSG_SYSEX_START) return;
    }

    switch (msg.type) {
        case MSG_NOTE_ON:
            if (msg.data2 == 0) {
                synth->keyUp(msg.data1);
                digitalWrite(LED_PIN, LOW);
            } else {
                synth->keyDown(msg.data1, msg.data2);
                digitalWrite(LED_PIN, HIGH);
            }
            break;

        case MSG_NOTE_OFF:
            synth->keyUp(msg.data1);
            digitalWrite(LED_PIN, LOW);
            break;

        case MSG_AFTERTOUCH_CHAN:
            synth->setAftertouch(msg.data1);
            break;

        case MSG_CONTROL_CHANGE:
            switch (msg.data1) {
                case 0:   synth->setVolume(msg.data2); break;  // Bank Select MSB (ignored, only LSB matters)
                case 1:   synth->setModWheel(msg.data2); break;
                case 2:   synth->setBreathController(msg.data2); break;
                case 4:   synth->setFootController(msg.data2); break;
                case 7:   synth->setVolume(msg.data2); break;
                case 32:  break; // Bank Select LSB - handled by voice_manager via callback
                case 64:  synth->setSustain(msg.data2 >= 64); break;
                case 65:  synth->setPortamento(msg.data2); break;
                case 95:  synth->setMasterTune(msg.data2); break;
                case 120: synth->panic(); break;
                case 123:
                    if (channel != 0) synth->notesOff();
                    break;
                case 126:
                    if (msg.data2 == 0) synth->setMonoMode(false);
                    break;
                case 127:
                    if (msg.data2 == 1) synth->setMonoMode(true);
                    break;
            }
            break;

        case MSG_PITCH_BEND:
            synth->setPitchBend(msg.data1, msg.data2);
            break;

        default:
            break;
    }
}

// ============================================================================
// SysEx Handler (Yamaha DX7 compatible)
// ============================================================================

void MIDIInput::handleSysEx(const uint8_t *data, uint16_t len) {
    if (!synth || len < 4) return;

    // Must be Yamaha manufacturer ID
    if (data[0] != MSG_SYSEX_START || data[1] != SYSEX_MANID_YAMAHA) return;

    uint8_t deviceNum = data[2] & 0x0F;
    if (deviceNum != MIDI_SYSEX_DEVICE_ID) return;
    if (data[len - 1] != MSG_SYSEX_END) return;

    int16_t result = synth->checkSystemExclusive(data, len);

    switch (result) {
        case 64:  synth->setMonoMode(data[5]); break;
        case 65:  synth->setPitchbendRange(data[5]); break;
        case 66:  synth->setPitchbendStep(data[5]); break;
        case 67:  synth->setPortamentoMode(data[5]); break;
        case 68:  synth->setPortamentoGlissando(data[5]); break;
        case 69:  synth->setPortamentoTime(data[5]); break;
        case 70:  synth->setModWheelRange(data[5]); break;
        case 71:  synth->setModWheelTarget(data[5]); break;
        case 72:  synth->setFootControllerRange(data[5]); break;
        case 73:  synth->setFootControllerTarget(data[5]); break;
        case 74:  synth->setBreathControllerRange(data[5]); break;
        case 75:  synth->setBreathControllerTarget(data[5]); break;
        case 76:  synth->setAftertouchRange(data[5]); break;
        case 77:  synth->setAftertouchTarget(data[5]); break;
        case 100:
            // Single voice SysEx upload
            synth->loadVoiceParameters(data);
            break;
        default:
            if (result >= 300 && result < 500) {
                synth->setVoiceDataElement(
                    data[4] + ((data[3] & 0x03) * 128), data[5]);
                if ((data[4] + ((data[3] & 0x03) * 128)) == 134) {
                    synth->notesOff();
                }
            }
            break;
    }
}
