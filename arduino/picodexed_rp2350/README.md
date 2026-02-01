# PicoDexed RP2350 - Arduino Port

DX7 FM Synthesizer for **Raspberry Pi Pico 2 (RP2350)**, ported to the Arduino
framework using Arduino Audio Tools for I2S output.

## RP2350 Advantages Over RP2040

| Feature | RP2040 (Original) | RP2350 (This Port) |
|---|---|---|
| CPU | Cortex-M0+ @ 133MHz | Cortex-M33 @ 150MHz |
| FPU | None (software float) | Hardware FPv5 (single-cycle) |
| SRAM | 264 KB | 520 KB |
| Flash | 2 MB | 4 MB (Pico 2) |
| Polyphony | 8-16 voices @ 24kHz | **24 voices @ 48kHz** |
| PIO Blocks | 2 (8 SM) | 3 (12 SM) |
| Audio Quality | 24 kHz / 16-bit | **48 kHz / 16-bit** |

The hardware FPU alone provides a ~6x speedup for the floating-point math used
in FM synthesis, allowing double the sample rate with triple the polyphony.

## Voice Storage (No SD Card Needed)

DX7 voices are parameter sets (128 bytes each), not audio samples. All 256
factory voices total just 32 KB. The Pico 2's 4 MB onboard flash is split:

- **1 MB** for sketch code
- **3 MB** for FatFS filesystem (voice bank storage)

That's room for **~750 voice banks** (24,000+ individual voices) with no
external storage hardware.

## Hardware Connections

Same pinout as the original picodexed:

```
I2S DAC (PCM5102A):          MIDI Input (DIN-5/TRS):
  GPIO 9  -> DIN               GPIO 5  -> RX (via optocoupler)
  GPIO 10 -> BCLK              GPIO 4  -> TX (optional)
  GPIO 11 -> LRCLK

SSD1306 OLED (I2C):          Rotary Encoder:
  GPIO 2  -> SDA               GPIO 6  -> A
  GPIO 3  -> SCL               GPIO 7  -> B
                                GPIO 8  -> Switch (optional)
```

## Setup

### 1. Install Arduino Board Package

In Arduino IDE, go to **File -> Preferences** and add this URL to
"Additional Boards Manager URLs":

```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

Then go to **Tools -> Board -> Boards Manager**, search for "pico", and install
**Raspberry Pi RP2040/RP2350 Boards**.

### 2. Install Libraries (One-Click)

**Windows:** Double-click **`install.bat`** in the sketch folder.

**Mac/Linux:** Run `./install.sh` in a terminal.

This downloads Synth_Dexed source files and Arduino Audio Tools automatically.

You still need to install these from **Sketch -> Include Library -> Manage Libraries**:
- Adafruit SSD1306 (click "Install All" when asked about dependencies)
- Adafruit GFX Library
- Adafruit BusIO
- Adafruit TinyUSB Library

### 3. Configure Arduino IDE

In the **Tools** menu, select:

| Setting | Value |
|---|---|
| Board | Raspberry Pi Pico 2 |
| CPU Architecture | ARM Cortex-M33 |
| Flash Size | 4MB (Sketch: 1MB, FS: 3MB) |
| USB Stack | Adafruit TinyUSB |

### 4. Compile and Upload

1. Connect Pico 2 via USB (hold BOOTSEL for first upload)
2. Click **Upload** in Arduino IDE
3. After first upload, subsequent uploads work without BOOTSEL

It works immediately with a built-in default voice (Brass 1).

### 5. Upload Voice Banks (Optional - Drag and Drop!)

To add more voices, use the **Voice Uploader** sketch:

1. Open **`voice_uploader/voice_uploader.ino`**
2. Change **USB Stack** to **"Pico SDK"** (NOT TinyUSB!)
3. Upload it to the Pico 2
4. The Pico 2 appears as a **USB drive** on your computer
5. Create a **`voices`** folder on the drive (if not already there)
6. **Drag and drop** your `.syx` files into the `voices` folder
7. **Safely eject** the drive
8. Open the **main sketch** (`picodexed_rp2350.ino`) again
9. Change **USB Stack** back to **"Adafruit TinyUSB"**
10. Upload the main sketch

Your voices stay on the flash between sketch uploads!

**Where to get .syx voice files:**
- https://yamahablackboxes.com/collection/yamaha-dx7-synthesizer/patches/
- Search Google for "DX7 SysEx patches" - thousands of free .syx files exist

## Architecture

```
Core 0:                          Core 1:
  MIDI Input (Serial + USB)        Synth_Dexed FM Engine
  Voice Manager                    I2S Audio Output
  OLED Display                     (Arduino Audio Tools)
  Rotary Encoder
         |                              |
         +--- mutex-protected Dexed ----+
```

Both cores share the Dexed synth engine, protected by a mutex. Core 0 sends
note events and controller changes; Core 1 continuously generates audio samples
and streams them to the I2S DAC.

## Credits

- [picodexed](https://github.com/diyelectromusic/picodexed) by diyelectromusic (Kevin)
- [Synth_Dexed](https://github.com/diyelectromusic/Synth_Dexed) - DX7 FM synthesis engine
- [Arduino Audio Tools](https://github.com/pschatzmann/arduino-audio-tools) by Phil Schatzmann
- [arduino-pico](https://github.com/earlephilhower/arduino-pico) by Earle F. Philhower III

## License

MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
