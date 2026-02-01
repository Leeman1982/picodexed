/*
 * PicoDexed RP2350 - Arduino Port
 * Dexed Interface Common - Arduino compatibility shim
 *
 * This replaces the Pico SDK version of dexed_if_common.h.
 * On Arduino, boolean/constrain/millis are already provided.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef _dexed_if_common_h
#define _dexed_if_common_h

#include <Arduino.h>

// signed_saturate_rshift - used by Synth_Dexed for sample clipping
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift)
{
    int32_t out, max;
    out = val >> rshift;
    max = 1 << (bits - 1);
    if (out >= 0) {
        if (out > max - 1) out = max - 1;
    } else {
        if (out < -max) out = -max;
    }
    return out;
}

#endif // _dexed_if_common_h
