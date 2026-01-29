#!/bin/bash
# ============================================================================
# PicoDexed RP2350 - Synth_Dexed Library Setup
# ============================================================================
#
# This script downloads and installs the Synth_Dexed library for Arduino.
# Run this once before compiling the sketch.
#
# Usage:
#   chmod +x setup_synth_dexed.sh
#   ./setup_synth_dexed.sh
#
# The library will be installed to your Arduino libraries folder.
# ============================================================================

set -e

# Detect Arduino libraries folder
if [ -d "$HOME/Arduino/libraries" ]; then
    LIBRARIES_DIR="$HOME/Arduino/libraries"
elif [ -d "$HOME/Documents/Arduino/libraries" ]; then
    LIBRARIES_DIR="$HOME/Documents/Arduino/libraries"
else
    echo "Could not find Arduino libraries folder."
    echo "Please specify the path:"
    read -r LIBRARIES_DIR
    if [ ! -d "$LIBRARIES_DIR" ]; then
        echo "Creating $LIBRARIES_DIR"
        mkdir -p "$LIBRARIES_DIR"
    fi
fi

echo "Arduino libraries folder: $LIBRARIES_DIR"

# Clone Synth_Dexed
SYNTH_DEXED_DIR="$LIBRARIES_DIR/Synth_Dexed"
if [ -d "$SYNTH_DEXED_DIR" ]; then
    echo "Synth_Dexed already exists, updating..."
    cd "$SYNTH_DEXED_DIR"
    git pull
else
    echo "Cloning Synth_Dexed..."
    git clone https://github.com/diyelectromusic/Synth_Dexed.git "$SYNTH_DEXED_DIR"
fi

echo ""
echo "============================================================================"
echo "Synth_Dexed installed to: $SYNTH_DEXED_DIR"
echo "============================================================================"
echo ""

# Check for other required libraries
echo "Checking for other required libraries..."
echo ""
echo "Please also install these via Arduino Library Manager (Sketch -> Include Library -> Manage Libraries):"
echo "  1. Adafruit SSD1306"
echo "  2. Adafruit GFX Library"
echo "  3. Adafruit BusIO"
echo "  4. Adafruit TinyUSB Library"
echo ""
echo "And install Arduino Audio Tools from GitHub:"
echo "  git clone https://github.com/pschatzmann/arduino-audio-tools.git $LIBRARIES_DIR/arduino-audio-tools"
echo ""

# Check if arduino-audio-tools is installed
if [ ! -d "$LIBRARIES_DIR/arduino-audio-tools" ]; then
    echo "Installing Arduino Audio Tools..."
    git clone https://github.com/pschatzmann/arduino-audio-tools.git "$LIBRARIES_DIR/arduino-audio-tools"
    echo "Arduino Audio Tools installed."
else
    echo "Arduino Audio Tools already installed."
fi

echo ""
echo "============================================================================"
echo "Setup complete! Open picodexed_rp2350.ino in Arduino IDE."
echo ""
echo "Arduino IDE Settings (Tools menu):"
echo "  Board:             Raspberry Pi Pico 2"
echo "  CPU Architecture:  ARM Cortex-M33"
echo "  Flash Size:        4MB (Sketch: 1MB, FS: 3MB)"
echo "  USB Stack:         Adafruit TinyUSB"
echo "============================================================================"
