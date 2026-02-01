/*
 * ============================================================================
 * PicoDexed RP2350 - Voice Bank Uploader
 * ============================================================================
 *
 * This tiny sketch turns your Pico 2 into a USB thumb drive.
 * Just drag and drop your .syx voice files onto it!
 *
 * HOW TO USE:
 * ============================================================================
 *
 *   Step 1: Upload this sketch to your Pico 2
 *           (use the settings below)
 *
 *   Step 2: The Pico 2 appears as a USB drive called "PICODEXED"
 *           on your computer
 *
 *   Step 3: Create a folder called "voices" on the drive
 *
 *   Step 4: Drag your .syx files into the "voices" folder
 *
 *   Step 5: Eject / Safely Remove the drive
 *
 *   Step 6: Upload the main picodexed_rp2350 sketch back
 *           (your voice files stay on the flash!)
 *
 * ============================================================================
 * ARDUINO IDE SETTINGS FOR THIS SKETCH (Tools menu):
 * ============================================================================
 *   Board:           Raspberry Pi Pico 2
 *   CPU Architecture: ARM Cortex-M33
 *   Flash Size:      4MB (Sketch: 1MB, FS: 3MB)  <-- IMPORTANT: same as main sketch!
 *   USB Stack:       Pico SDK                     <-- NOTE: Pico SDK, NOT TinyUSB!
 *
 * ============================================================================
 * WHERE TO GET VOICE FILES:
 * ============================================================================
 *   - https://yamahablackboxes.com/collection/yamaha-dx7-synthesizer/patches/
 *   - https://homepages.abdn.ac.uk/d.j.benson/pages/dx7/mangled.html
 *   - Search for "DX7 SysEx patches" - thousands of free .syx files exist
 *
 *   Each .syx file = 1 bank of 32 voices. A 3MB filesystem holds ~750 banks.
 *
 * ============================================================================
 */

#include <FatFS.h>
#include <FatFSUSB.h>

FatFSUSB driveUSB;

// Blink pattern to show it's alive
void blink(int times, int ms) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(ms);
        digitalWrite(LED_BUILTIN, LOW);
        delay(ms);
    }
}

void setup() {
    // LED setup
    pinMode(LED_BUILTIN, OUTPUT);

    // Format and mount FatFS on the flash filesystem partition
    // Uses the same flash region as LittleFS (set by Flash Size in Tools menu)
    if (!FatFS.begin()) {
        // First time? Format the filesystem
        FatFS.format();
        if (!FatFS.begin()) {
            // Format failed - blink SOS pattern
            while (1) {
                blink(3, 100);  // S
                blink(3, 300);  // O
                blink(3, 100);  // S
                delay(1000);
            }
        }
    }

    // Create the voices directory if it doesn't exist
    FatFS.mkdir("/voices");

    // Start USB Mass Storage - Pico 2 now appears as a USB drive!
    driveUSB.begin();

    // Three quick blinks = ready
    blink(3, 150);

    // Keep LED on solid to show USB drive mode is active
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    // Nothing to do - the USB Mass Storage runs automatically.
    // The LED stays on to show it's in USB drive mode.
    // When you're done copying files, just upload the main sketch.
    delay(100);
}
