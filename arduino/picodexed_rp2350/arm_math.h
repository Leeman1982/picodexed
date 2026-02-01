/*
 * PicoDexed RP2350 - Arduino Port
 * ARM Math Compatibility Layer
 *
 * Provides arm_math.h types and functions needed by Synth_Dexed.
 * On RP2350 (Cortex-M33), these map to hardware FPU operations.
 *
 * Based on picodexed by diyelectromusic (Kevin)
 * MIT License - Copyright (c) 2025 diyelectromusic (Kevin)
 */
#ifndef ARM_MATH_COMPAT_H
#define ARM_MATH_COMPAT_H

#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Type definitions matching ARM CMSIS-DSP
// ============================================================================
typedef float    float32_t;
typedef int16_t  q15_t;
typedef int32_t  q31_t;

// ============================================================================
// Saturating shift (replaces ARM __SSAT intrinsic)
// ============================================================================
static inline q31_t __SSAT(q31_t x, uint32_t y) {
    int32_t posMax = 1;
    for (uint32_t i = 0; i < (y - 1); i++) {
        posMax *= 2;
    }
    if (x > 0) {
        if (x > (posMax - 1)) x = posMax - 1;
    } else {
        if (x < -posMax) x = -posMax;
    }
    return x;
}

// ============================================================================
// Float-to-Q15 conversion
// On RP2350, float multiply uses hardware FPU (single-cycle)
// ============================================================================
static inline void arm_float_to_q15(
    const float32_t *pSrc, q15_t *pDst, uint32_t blockSize)
{
    for (uint32_t i = 0; i < blockSize; i++) {
        float32_t val = pSrc[i] * 32768.0f;
        if (val > 32767.0f) val = 32767.0f;
        if (val < -32768.0f) val = -32768.0f;
        pDst[i] = (q15_t)val;
    }
}

// ============================================================================
// Vector fill
// ============================================================================
static inline void arm_fill_f32(
    float32_t value, float32_t *pDst, uint32_t blockSize)
{
    for (uint32_t i = 0; i < blockSize; i++) {
        pDst[i] = value;
    }
}

// ============================================================================
// Vector subtract: pDst[i] = pSrcA[i] - pSrcB[i]
// ============================================================================
static inline void arm_sub_f32(
    const float32_t *pSrcA, const float32_t *pSrcB,
    float32_t *pDst, uint32_t blockSize)
{
    for (uint32_t i = 0; i < blockSize; i++) {
        pDst[i] = pSrcA[i] - pSrcB[i];
    }
}

// ============================================================================
// Vector scale: pDst[i] = pSrc[i] * scale
// ============================================================================
static inline void arm_scale_f32(
    const float32_t *pSrc, float32_t scale,
    float32_t *pDst, uint32_t blockSize)
{
    for (uint32_t i = 0; i < blockSize; i++) {
        pDst[i] = pSrc[i] * scale;
    }
}

// ============================================================================
// Vector offset: pDst[i] = pSrc[i] + offset
// ============================================================================
static inline void arm_offset_f32(
    const float32_t *pSrc, float32_t offset,
    float32_t *pDst, uint32_t blockSize)
{
    for (uint32_t i = 0; i < blockSize; i++) {
        pDst[i] = pSrc[i] + offset;
    }
}

// ============================================================================
// Vector multiply: pDst[i] = pSrcA[i] * pSrcB[i]
// ============================================================================
static inline void arm_mult_f32(
    const float32_t *pSrcA, const float32_t *pSrcB,
    float32_t *pDst, uint32_t blockSize)
{
    for (uint32_t i = 0; i < blockSize; i++) {
        pDst[i] = pSrcA[i] * pSrcB[i];
    }
}

// ============================================================================
// Biquad cascade filter (Direct Form I)
// ============================================================================
typedef struct {
    uint32_t   numStages;
    float32_t *pState;
    float32_t *pCoeffs;
} arm_biquad_casd_df1_inst_f32;

static inline void arm_biquad_cascade_df1_f32(
    const arm_biquad_casd_df1_inst_f32 *S,
    const float32_t *pSrc, float32_t *pDst, uint32_t blockSize)
{
    float32_t *pState  = S->pState;
    float32_t *pCoeffs = S->pCoeffs;

    for (uint32_t stage = 0; stage < S->numStages; stage++) {
        float32_t b0 = pCoeffs[0];
        float32_t b1 = pCoeffs[1];
        float32_t b2 = pCoeffs[2];
        float32_t a1 = pCoeffs[3];
        float32_t a2 = pCoeffs[4];

        float32_t x_n1 = pState[0];
        float32_t x_n2 = pState[1];
        float32_t y_n1 = pState[2];
        float32_t y_n2 = pState[3];

        for (uint32_t i = 0; i < blockSize; i++) {
            float32_t x = (stage == 0) ? pSrc[i] : pDst[i];
            float32_t y = b0 * x + b1 * x_n1 + b2 * x_n2 - a1 * y_n1 - a2 * y_n2;
            x_n2 = x_n1;
            x_n1 = x;
            y_n2 = y_n1;
            y_n1 = y;
            pDst[i] = y;
        }

        pState[0] = x_n1;
        pState[1] = x_n2;
        pState[2] = y_n1;
        pState[3] = y_n2;

        pState  += 4;
        pCoeffs += 5;
    }
}

static inline void arm_biquad_cascade_df1_init_f32(
    arm_biquad_casd_df1_inst_f32 *S,
    uint8_t numStages, const float32_t *pCoeffs, float32_t *pState)
{
    S->numStages = numStages;
    S->pCoeffs   = (float32_t *)pCoeffs;
    S->pState    = pState;
    for (uint32_t i = 0; i < 4u * numStages; i++) {
        pState[i] = 0.0f;
    }
}

#ifdef __cplusplus
}
#endif

#endif // ARM_MATH_COMPAT_H
