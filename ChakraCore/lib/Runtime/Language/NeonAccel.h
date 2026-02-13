//-------------------------------------------------------------------------------------------------------
// Copyright (C) ChakraSilicon Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NeonAccel.h
//
// Header-only NEON acceleration library for ChakraSilicon.
// Provides vectorized implementations of common array operations using ARM NEON intrinsics.
//
// Phase 1 of the NEON for ChakraSilicon acceleration plan.
// All functions are inlined and gated on CHAKRA_NEON_AVAILABLE.
// When NEON is not available, callers fall back to their existing scalar paths.
//
// Operations provided:
//   - Fill (all TypedArray element types)
//   - IndexOf / Search (int8, int16, int32, uint32, float32, float64)
//   - Min/Max scan (int32, uint32, float32, float64)
//   - Reverse (int8/uint8, int16/uint16, int32/uint32/float32, float64)
//
//-------------------------------------------------------------------------------------------------------

#pragma once

#if (defined(__aarch64__) || defined(_M_ARM64)) && !defined(CHAKRA_NEON_DISABLED)
#include <arm_neon.h>
#define CHAKRA_NEON_AVAILABLE 1
#else
#define CHAKRA_NEON_AVAILABLE 0
#endif

#if CHAKRA_NEON_AVAILABLE

#include <cstring>   // memcpy
#include <cmath>     // isnan
#include <type_traits> // is_signed

namespace NeonAccel
{

// =====================================================================================
// Section 1: Fill Operations
//
// NEON-accelerated fill for all TypedArray element types.
// Each function splats a value into a NEON register and writes 128-bit stores
// in an unrolled main loop (4 stores = 64 bytes per iteration for 32-bit types),
// then handles tail elements.
// =====================================================================================

// --- Float32 Fill ---
// 4 floats per 128-bit register, unrolled 4x = 16 floats per iteration
inline void NeonFillFloat32(float* dst, uint32_t count, float value)
{
    float32x4_t vval = vdupq_n_f32(value);
    uint32_t i = 0;

    // Main loop: 16 floats per iteration
    for (; i + 15 < count; i += 16)
    {
        vst1q_f32(dst + i,      vval);
        vst1q_f32(dst + i + 4,  vval);
        vst1q_f32(dst + i + 8,  vval);
        vst1q_f32(dst + i + 12, vval);
    }
    // 4-element tail
    for (; i + 3 < count; i += 4)
    {
        vst1q_f32(dst + i, vval);
    }
    // Scalar tail
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

// --- Float64 Fill ---
// 2 doubles per 128-bit register, unrolled 4x = 8 doubles per iteration
inline void NeonFillFloat64(double* dst, uint32_t count, double value)
{
    float64x2_t vval = vdupq_n_f64(value);
    uint32_t i = 0;

    for (; i + 7 < count; i += 8)
    {
        vst1q_f64(dst + i,     vval);
        vst1q_f64(dst + i + 2, vval);
        vst1q_f64(dst + i + 4, vval);
        vst1q_f64(dst + i + 6, vval);
    }
    for (; i + 1 < count; i += 2)
    {
        vst1q_f64(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

// --- Int32 / Uint32 Fill ---
// 4 int32s per 128-bit register, unrolled 4x = 16 elements per iteration
inline void NeonFillInt32(int32_t* dst, uint32_t count, int32_t value)
{
    int32x4_t vval = vdupq_n_s32(value);
    uint32_t i = 0;

    for (; i + 15 < count; i += 16)
    {
        vst1q_s32(dst + i,      vval);
        vst1q_s32(dst + i + 4,  vval);
        vst1q_s32(dst + i + 8,  vval);
        vst1q_s32(dst + i + 12, vval);
    }
    for (; i + 3 < count; i += 4)
    {
        vst1q_s32(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

inline void NeonFillUint32(uint32_t* dst, uint32_t count, uint32_t value)
{
    uint32x4_t vval = vdupq_n_u32(value);
    uint32_t i = 0;

    for (; i + 15 < count; i += 16)
    {
        vst1q_u32(dst + i,      vval);
        vst1q_u32(dst + i + 4,  vval);
        vst1q_u32(dst + i + 8,  vval);
        vst1q_u32(dst + i + 12, vval);
    }
    for (; i + 3 < count; i += 4)
    {
        vst1q_u32(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

// --- Int16 / Uint16 Fill ---
// 8 int16s per 128-bit register, unrolled 4x = 32 elements per iteration
inline void NeonFillInt16(int16_t* dst, uint32_t count, int16_t value)
{
    int16x8_t vval = vdupq_n_s16(value);
    uint32_t i = 0;

    for (; i + 31 < count; i += 32)
    {
        vst1q_s16(dst + i,      vval);
        vst1q_s16(dst + i + 8,  vval);
        vst1q_s16(dst + i + 16, vval);
        vst1q_s16(dst + i + 24, vval);
    }
    for (; i + 7 < count; i += 8)
    {
        vst1q_s16(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

inline void NeonFillUint16(uint16_t* dst, uint32_t count, uint16_t value)
{
    uint16x8_t vval = vdupq_n_u16(value);
    uint32_t i = 0;

    for (; i + 31 < count; i += 32)
    {
        vst1q_u16(dst + i,      vval);
        vst1q_u16(dst + i + 8,  vval);
        vst1q_u16(dst + i + 16, vval);
        vst1q_u16(dst + i + 24, vval);
    }
    for (; i + 7 < count; i += 8)
    {
        vst1q_u16(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

// --- Int8 / Uint8 Fill ---
// 16 int8s per 128-bit register, unrolled 4x = 64 elements per iteration
inline void NeonFillInt8(int8_t* dst, uint32_t count, int8_t value)
{
    int8x16_t vval = vdupq_n_s8(value);
    uint32_t i = 0;

    for (; i + 63 < count; i += 64)
    {
        vst1q_s8(dst + i,      vval);
        vst1q_s8(dst + i + 16, vval);
        vst1q_s8(dst + i + 32, vval);
        vst1q_s8(dst + i + 48, vval);
    }
    for (; i + 15 < count; i += 16)
    {
        vst1q_s8(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}

inline void NeonFillUint8(uint8_t* dst, uint32_t count, uint8_t value)
{
    uint8x16_t vval = vdupq_n_u8(value);
    uint32_t i = 0;

    for (; i + 63 < count; i += 64)
    {
        vst1q_u8(dst + i,      vval);
        vst1q_u8(dst + i + 16, vval);
        vst1q_u8(dst + i + 32, vval);
        vst1q_u8(dst + i + 48, vval);
    }
    for (; i + 15 < count; i += 16)
    {
        vst1q_u8(dst + i, vval);
    }
    for (; i < count; i++)
    {
        dst[i] = value;
    }
}


// =====================================================================================
// Section 2: IndexOf / Search Operations
//
// NEON-accelerated linear search for TypedArray indexOf/includes.
// Each function broadcasts the target value, loads chunks, does a vector compare,
// checks if any lane matched, then finds the exact lane on hit.
// Early-exit on first match preserves O(1) best-case.
// =====================================================================================

// --- Int32 IndexOf ---
// 4 int32s per 128-bit compare
inline int32_t NeonIndexOfInt32(const int32_t* buf, uint32_t len, int32_t target)
{
    int32x4_t vtarget = vdupq_n_s32(target);
    uint32_t i = 0;

    for (; i + 3 < len; i += 4)
    {
        int32x4_t chunk = vld1q_s32(buf + i);
        uint32x4_t cmp = vceqq_s32(chunk, vtarget);

        // Horizontal reduce: check if any lane is non-zero
        uint32x2_t reduced = vorr_u32(vget_low_u32(cmp), vget_high_u32(cmp));
        if (vget_lane_u32(vpmax_u32(reduced, reduced), 0) != 0)
        {
            // Find exact lane
            for (uint32_t j = 0; j < 4 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    // Scalar tail
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Uint32 IndexOf ---
inline int32_t NeonIndexOfUint32(const uint32_t* buf, uint32_t len, uint32_t target)
{
    uint32x4_t vtarget = vdupq_n_u32(target);
    uint32_t i = 0;

    for (; i + 3 < len; i += 4)
    {
        uint32x4_t chunk = vld1q_u32(buf + i);
        uint32x4_t cmp = vceqq_u32(chunk, vtarget);

        uint32x2_t reduced = vorr_u32(vget_low_u32(cmp), vget_high_u32(cmp));
        if (vget_lane_u32(vpmax_u32(reduced, reduced), 0) != 0)
        {
            for (uint32_t j = 0; j < 4 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Float32 IndexOf ---
// Note: NaN never equals NaN per IEEE 754 (and vceqq_f32 follows this),
// so NaN targets will correctly never match.
// -0.0 == +0.0 is true for vceqq_f32 as per IEEE, which matches JS semantics for indexOf.
inline int32_t NeonIndexOfFloat32(const float* buf, uint32_t len, float target)
{
    float32x4_t vtarget = vdupq_n_f32(target);
    uint32_t i = 0;

    for (; i + 3 < len; i += 4)
    {
        float32x4_t chunk = vld1q_f32(buf + i);
        uint32x4_t cmp = vceqq_f32(chunk, vtarget);

        uint32x2_t reduced = vorr_u32(vget_low_u32(cmp), vget_high_u32(cmp));
        if (vget_lane_u32(vpmax_u32(reduced, reduced), 0) != 0)
        {
            for (uint32_t j = 0; j < 4 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Float64 IndexOf ---
inline int32_t NeonIndexOfFloat64(const double* buf, uint32_t len, double target)
{
    float64x2_t vtarget = vdupq_n_f64(target);
    uint32_t i = 0;

    for (; i + 1 < len; i += 2)
    {
        float64x2_t chunk = vld1q_f64(buf + i);
        uint64x2_t cmp = vceqq_f64(chunk, vtarget);

        uint64x1_t reduced = vorr_u64(vget_low_u64(cmp), vget_high_u64(cmp));
        if (vget_lane_u64(reduced, 0) != 0)
        {
            for (uint32_t j = 0; j < 2 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Int16 IndexOf ---
// 8 int16s per 128-bit compare
inline int32_t NeonIndexOfInt16(const int16_t* buf, uint32_t len, int16_t target)
{
    int16x8_t vtarget = vdupq_n_s16(target);
    uint32_t i = 0;

    for (; i + 7 < len; i += 8)
    {
        int16x8_t chunk = vld1q_s16(buf + i);
        uint16x8_t cmp = vceqq_s16(chunk, vtarget);

        // Reduce: narrow to 8-bit, then check
        uint8x8_t narrowed = vmovn_u16(cmp);
        uint64x1_t collapsed = vreinterpret_u64_u8(narrowed);
        if (vget_lane_u64(collapsed, 0) != 0)
        {
            for (uint32_t j = 0; j < 8 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Int8 IndexOf ---
// 16 int8s per 128-bit compare
inline int32_t NeonIndexOfInt8(const int8_t* buf, uint32_t len, int8_t target)
{
    int8x16_t vtarget = vdupq_n_s8(target);
    uint32_t i = 0;

    for (; i + 15 < len; i += 16)
    {
        int8x16_t chunk = vld1q_s8(buf + i);
        uint8x16_t cmp = vceqq_s8(chunk, vtarget);

        // Check if any byte is non-zero using UMAXV
        uint8_t maxVal = vmaxvq_u8(cmp);
        if (maxVal != 0)
        {
            for (uint32_t j = 0; j < 16 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Uint8 IndexOf ---
inline int32_t NeonIndexOfUint8(const uint8_t* buf, uint32_t len, uint8_t target)
{
    uint8x16_t vtarget = vdupq_n_u8(target);
    uint32_t i = 0;

    for (; i + 15 < len; i += 16)
    {
        uint8x16_t chunk = vld1q_u8(buf + i);
        uint8x16_t cmp = vceqq_u8(chunk, vtarget);

        uint8_t maxVal = vmaxvq_u8(cmp);
        if (maxVal != 0)
        {
            for (uint32_t j = 0; j < 16 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}

// --- Uint16 IndexOf ---
inline int32_t NeonIndexOfUint16(const uint16_t* buf, uint32_t len, uint16_t target)
{
    uint16x8_t vtarget = vdupq_n_u16(target);
    uint32_t i = 0;

    for (; i + 7 < len; i += 8)
    {
        uint16x8_t chunk = vld1q_u16(buf + i);
        uint16x8_t cmp = vceqq_u16(chunk, vtarget);

        uint8x8_t narrowed = vmovn_u16(cmp);
        uint64x1_t collapsed = vreinterpret_u64_u8(narrowed);
        if (vget_lane_u64(collapsed, 0) != 0)
        {
            for (uint32_t j = 0; j < 8 && (i + j) < len; j++)
            {
                if (buf[i + j] == target) return (int32_t)(i + j);
            }
        }
    }
    for (; i < len; i++)
    {
        if (buf[i] == target) return (int32_t)i;
    }
    return -1;
}


// =====================================================================================
// Section 3: Min/Max Scan Operations
//
// NEON-accelerated min/max reduction for TypedArray FindMinOrMax.
//
// Integer variants: straightforward NEON min/max + horizontal reduce.
// Float variants: must handle NaN propagation and -0/+0 ordering per JS spec.
//   JS spec: "If any value is NaN, return NaN."
//   JS spec: -0 < +0 for min, +0 > -0 for max.
//
// Strategy for floats:
//   1. First check for NaN with a quick NEON scan (compare each element to itself;
//      NaN != NaN). If any NaN found, return NaN immediately.
//   2. Use NEON vminq/vmaxq for the bulk scan (these handle NaN by propagation
//      on ARMv8, but the -0/+0 case needs a fixup pass).
//   3. Track -0/+0 separately during the scalar fixup at the end.
// =====================================================================================

// --- Float32 Min/Max ---
// Returns the minimum or maximum float value in the buffer.
// Handles NaN (returns NaN if any element is NaN) and -0/+0 ordering.
inline float NeonMinMaxFloat32(const float* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0.0f; // Should not happen in practice
    if (len == 1) return buf[0];

    // Quick NaN scan + initial min/max using NEON
    uint32_t i = 0;
    float currentResult = buf[0];

    // Check first element for NaN
    if (std::isnan(currentResult))
    {
        return currentResult;
    }

    if (len >= 4)
    {
        float32x4_t vaccum = vld1q_f32(buf);

        // Check initial chunk for NaN: NaN != NaN
        uint32x4_t nanCheck = vceqq_f32(vaccum, vaccum);
        // If any lane is NOT equal to itself, it's NaN
        uint32x2_t nanReduced = vand_u32(vget_low_u32(nanCheck), vget_high_u32(nanCheck));
        if (vget_lane_u32(vpmin_u32(nanReduced, nanReduced), 0) == 0)
        {
            // At least one NaN in first chunk - find it
            for (uint32_t j = 0; j < 4; j++)
            {
                if (std::isnan(buf[j])) return buf[j];
            }
        }

        i = 4;
        for (; i + 3 < len; i += 4)
        {
            float32x4_t chunk = vld1q_f32(buf + i);

            // NaN check
            uint32x4_t chunkNanCheck = vceqq_f32(chunk, chunk);
            uint32x2_t chunkNanReduced = vand_u32(vget_low_u32(chunkNanCheck), vget_high_u32(chunkNanCheck));
            if (vget_lane_u32(vpmin_u32(chunkNanReduced, chunkNanReduced), 0) == 0)
            {
                for (uint32_t j = 0; j < 4 && (i + j) < len; j++)
                {
                    if (std::isnan(buf[i + j])) return buf[i + j];
                }
            }

            if (findMax)
            {
                vaccum = vmaxq_f32(vaccum, chunk);
            }
            else
            {
                vaccum = vminq_f32(vaccum, chunk);
            }
        }

        // Horizontal reduce the accumulator
        if (findMax)
        {
            float32x2_t half = vpmax_f32(vget_low_f32(vaccum), vget_high_f32(vaccum));
            half = vpmax_f32(half, half);
            currentResult = vget_lane_f32(half, 0);
        }
        else
        {
            float32x2_t half = vpmin_f32(vget_low_f32(vaccum), vget_high_f32(vaccum));
            half = vpmin_f32(half, half);
            currentResult = vget_lane_f32(half, 0);
        }

        // The NEON min/max might not handle -0/+0 correctly per JS spec.
        // Do a scalar re-scan of the accumulator lanes to fix up -0/+0.
        // We need to check if the result is zero and if any lane was -0.
        if (currentResult == 0.0f)
        {
            float lanes[4];
            vst1q_f32(lanes, vaccum);
            bool hasNegZero = false;
            bool hasPosZero = false;
            for (int k = 0; k < 4; k++)
            {
                if (lanes[k] == 0.0f)
                {
                    uint32_t bits;
                    memcpy(&bits, &lanes[k], sizeof(bits));
                    if (bits & 0x80000000u)
                        hasNegZero = true;
                    else
                        hasPosZero = true;
                }
            }
            if (hasNegZero && hasPosZero)
            {
                // For min: -0 < +0; for max: +0 > -0
                if (findMax)
                    currentResult = 0.0f; // +0
                else
                {
                    currentResult = -0.0f;
                }
            }
        }
    }
    else
    {
        i = 1;
    }

    // Scalar tail (also handles -0/+0 correctly)
    for (; i < len; i++)
    {
        float val = buf[i];
        if (std::isnan(val)) return val;

        if (findMax)
        {
            if (val > currentResult)
            {
                currentResult = val;
            }
            else if (val == currentResult && val == 0.0f)
            {
                // +0 > -0
                uint32_t valBits, resBits;
                memcpy(&valBits, &val, sizeof(valBits));
                memcpy(&resBits, &currentResult, sizeof(resBits));
                if ((resBits & 0x80000000u) && !(valBits & 0x80000000u))
                {
                    currentResult = val; // val is +0, currentResult is -0
                }
            }
        }
        else
        {
            if (val < currentResult)
            {
                currentResult = val;
            }
            else if (val == currentResult && val == 0.0f)
            {
                // -0 < +0
                uint32_t valBits, resBits;
                memcpy(&valBits, &val, sizeof(valBits));
                memcpy(&resBits, &currentResult, sizeof(resBits));
                if (!(resBits & 0x80000000u) && (valBits & 0x80000000u))
                {
                    currentResult = val; // val is -0, currentResult is +0
                }
            }
        }
    }

    return currentResult;
}

// --- Float64 Min/Max ---
inline double NeonMinMaxFloat64(const double* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0.0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    double currentResult = buf[0];

    if (std::isnan(currentResult))
    {
        return currentResult;
    }

    if (len >= 2)
    {
        float64x2_t vaccum = vld1q_f64(buf);

        // NaN check on initial
        uint64x2_t nanCheck = vceqq_f64(vaccum, vaccum);
        if ((vgetq_lane_u64(nanCheck, 0) == 0) || (vgetq_lane_u64(nanCheck, 1) == 0))
        {
            for (uint32_t j = 0; j < 2; j++)
            {
                if (std::isnan(buf[j])) return buf[j];
            }
        }

        i = 2;
        for (; i + 1 < len; i += 2)
        {
            float64x2_t chunk = vld1q_f64(buf + i);

            uint64x2_t chunkNanCheck = vceqq_f64(chunk, chunk);
            if ((vgetq_lane_u64(chunkNanCheck, 0) == 0) || (vgetq_lane_u64(chunkNanCheck, 1) == 0))
            {
                for (uint32_t j = 0; j < 2 && (i + j) < len; j++)
                {
                    if (std::isnan(buf[i + j])) return buf[i + j];
                }
            }

            if (findMax)
            {
                vaccum = vmaxq_f64(vaccum, chunk);
            }
            else
            {
                vaccum = vminq_f64(vaccum, chunk);
            }
        }

        // Horizontal reduce: 2 lanes
        double lane0 = vgetq_lane_f64(vaccum, 0);
        double lane1 = vgetq_lane_f64(vaccum, 1);

        if (findMax)
        {
            if (lane0 > lane1)
                currentResult = lane0;
            else if (lane1 > lane0)
                currentResult = lane1;
            else if (lane0 == 0.0 && lane1 == 0.0)
            {
                // Check -0/+0
                uint64_t bits0, bits1;
                memcpy(&bits0, &lane0, sizeof(bits0));
                memcpy(&bits1, &lane1, sizeof(bits1));
                currentResult = ((bits0 & 0x8000000000000000ULL) && !(bits1 & 0x8000000000000000ULL)) ? lane1 : lane0;
            }
            else
            {
                currentResult = lane0;
            }
        }
        else
        {
            if (lane0 < lane1)
                currentResult = lane0;
            else if (lane1 < lane0)
                currentResult = lane1;
            else if (lane0 == 0.0 && lane1 == 0.0)
            {
                uint64_t bits0, bits1;
                memcpy(&bits0, &lane0, sizeof(bits0));
                memcpy(&bits1, &lane1, sizeof(bits1));
                currentResult = (!(bits0 & 0x8000000000000000ULL) && (bits1 & 0x8000000000000000ULL)) ? lane1 : lane0;
            }
            else
            {
                currentResult = lane0;
            }
        }
    }
    else
    {
        i = 1;
    }

    // Scalar tail
    for (; i < len; i++)
    {
        double val = buf[i];
        if (std::isnan(val)) return val;

        if (findMax)
        {
            if (val > currentResult)
            {
                currentResult = val;
            }
            else if (val == currentResult && val == 0.0)
            {
                uint64_t valBits, resBits;
                memcpy(&valBits, &val, sizeof(valBits));
                memcpy(&resBits, &currentResult, sizeof(resBits));
                if ((resBits & 0x8000000000000000ULL) && !(valBits & 0x8000000000000000ULL))
                {
                    currentResult = val;
                }
            }
        }
        else
        {
            if (val < currentResult)
            {
                currentResult = val;
            }
            else if (val == currentResult && val == 0.0)
            {
                uint64_t valBits, resBits;
                memcpy(&valBits, &val, sizeof(valBits));
                memcpy(&resBits, &currentResult, sizeof(resBits));
                if (!(resBits & 0x8000000000000000ULL) && (valBits & 0x8000000000000000ULL))
                {
                    currentResult = val;
                }
            }
        }
    }

    return currentResult;
}

// --- Int32 Min/Max ---
// No NaN or -0 concerns for integers.
inline int32_t NeonMinMaxInt32(const int32_t* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    int32_t currentResult;

    if (len >= 4)
    {
        int32x4_t vaccum = vld1q_s32(buf);
        i = 4;

        for (; i + 3 < len; i += 4)
        {
            int32x4_t chunk = vld1q_s32(buf + i);
            if (findMax)
                vaccum = vmaxq_s32(vaccum, chunk);
            else
                vaccum = vminq_s32(vaccum, chunk);
        }

        // Horizontal reduce
        if (findMax)
            currentResult = vmaxvq_s32(vaccum);
        else
            currentResult = vminvq_s32(vaccum);
    }
    else
    {
        currentResult = buf[0];
        i = 1;
    }

    // Scalar tail
    for (; i < len; i++)
    {
        if (findMax)
        {
            if (buf[i] > currentResult) currentResult = buf[i];
        }
        else
        {
            if (buf[i] < currentResult) currentResult = buf[i];
        }
    }

    return currentResult;
}

// --- Uint32 Min/Max ---
inline uint32_t NeonMinMaxUint32(const uint32_t* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    uint32_t currentResult;

    if (len >= 4)
    {
        uint32x4_t vaccum = vld1q_u32(buf);
        i = 4;

        for (; i + 3 < len; i += 4)
        {
            uint32x4_t chunk = vld1q_u32(buf + i);
            if (findMax)
                vaccum = vmaxq_u32(vaccum, chunk);
            else
                vaccum = vminq_u32(vaccum, chunk);
        }

        if (findMax)
            currentResult = vmaxvq_u32(vaccum);
        else
            currentResult = vminvq_u32(vaccum);
    }
    else
    {
        currentResult = buf[0];
        i = 1;
    }

    for (; i < len; i++)
    {
        if (findMax)
        {
            if (buf[i] > currentResult) currentResult = buf[i];
        }
        else
        {
            if (buf[i] < currentResult) currentResult = buf[i];
        }
    }

    return currentResult;
}

// --- Int16 Min/Max ---
inline int16_t NeonMinMaxInt16(const int16_t* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    int16_t currentResult;

    if (len >= 8)
    {
        int16x8_t vaccum = vld1q_s16(buf);
        i = 8;

        for (; i + 7 < len; i += 8)
        {
            int16x8_t chunk = vld1q_s16(buf + i);
            if (findMax)
                vaccum = vmaxq_s16(vaccum, chunk);
            else
                vaccum = vminq_s16(vaccum, chunk);
        }

        if (findMax)
            currentResult = vmaxvq_s16(vaccum);
        else
            currentResult = vminvq_s16(vaccum);
    }
    else
    {
        currentResult = buf[0];
        i = 1;
    }

    for (; i < len; i++)
    {
        if (findMax)
        {
            if (buf[i] > currentResult) currentResult = buf[i];
        }
        else
        {
            if (buf[i] < currentResult) currentResult = buf[i];
        }
    }

    return currentResult;
}

// --- Uint16 Min/Max ---
inline uint16_t NeonMinMaxUint16(const uint16_t* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    uint16_t currentResult;

    if (len >= 8)
    {
        uint16x8_t vaccum = vld1q_u16(buf);
        i = 8;

        for (; i + 7 < len; i += 8)
        {
            uint16x8_t chunk = vld1q_u16(buf + i);
            if (findMax)
                vaccum = vmaxq_u16(vaccum, chunk);
            else
                vaccum = vminq_u16(vaccum, chunk);
        }

        if (findMax)
            currentResult = vmaxvq_u16(vaccum);
        else
            currentResult = vminvq_u16(vaccum);
    }
    else
    {
        currentResult = buf[0];
        i = 1;
    }

    for (; i < len; i++)
    {
        if (findMax)
        {
            if (buf[i] > currentResult) currentResult = buf[i];
        }
        else
        {
            if (buf[i] < currentResult) currentResult = buf[i];
        }
    }

    return currentResult;
}

// --- Int8 Min/Max ---
inline int8_t NeonMinMaxInt8(const int8_t* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    int8_t currentResult;

    if (len >= 16)
    {
        int8x16_t vaccum = vld1q_s8(buf);
        i = 16;

        for (; i + 15 < len; i += 16)
        {
            int8x16_t chunk = vld1q_s8(buf + i);
            if (findMax)
                vaccum = vmaxq_s8(vaccum, chunk);
            else
                vaccum = vminq_s8(vaccum, chunk);
        }

        if (findMax)
            currentResult = vmaxvq_s8(vaccum);
        else
            currentResult = vminvq_s8(vaccum);
    }
    else
    {
        currentResult = buf[0];
        i = 1;
    }

    for (; i < len; i++)
    {
        if (findMax)
        {
            if (buf[i] > currentResult) currentResult = buf[i];
        }
        else
        {
            if (buf[i] < currentResult) currentResult = buf[i];
        }
    }

    return currentResult;
}

// --- Uint8 Min/Max ---
inline uint8_t NeonMinMaxUint8(const uint8_t* buf, uint32_t len, bool findMax)
{
    if (len == 0) return 0;
    if (len == 1) return buf[0];

    uint32_t i = 0;
    uint8_t currentResult;

    if (len >= 16)
    {
        uint8x16_t vaccum = vld1q_u8(buf);
        i = 16;

        for (; i + 15 < len; i += 16)
        {
            uint8x16_t chunk = vld1q_u8(buf + i);
            if (findMax)
                vaccum = vmaxq_u8(vaccum, chunk);
            else
                vaccum = vminq_u8(vaccum, chunk);
        }

        if (findMax)
            currentResult = vmaxvq_u8(vaccum);
        else
            currentResult = vminvq_u8(vaccum);
    }
    else
    {
        currentResult = buf[0];
        i = 1;
    }

    for (; i < len; i++)
    {
        if (findMax)
        {
            if (buf[i] > currentResult) currentResult = buf[i];
        }
        else
        {
            if (buf[i] < currentResult) currentResult = buf[i];
        }
    }

    return currentResult;
}


// =====================================================================================
// Section 4: Array Reverse Operations
//
// NEON-accelerated in-place reversal for TypedArray elements.
// Strategy: load chunks from front and back, reverse each chunk using NEON
// shuffle/rev instructions, then store them crossed (front->back, back->front).
// A scalar loop handles the remaining middle elements.
// =====================================================================================

// --- Float32 Reverse ---
inline void NeonReverseFloat32(float* buf, uint32_t len)
{
    if (len <= 1) return;

    uint32_t lo = 0;
    uint32_t hi = len - 4;

    while (lo + 3 < hi)
    {
        float32x4_t front = vld1q_f32(buf + lo);
        float32x4_t back  = vld1q_f32(buf + hi);

        // Reverse each 4-element chunk:
        // vrev64q swaps pairs within 64-bit halves: [0,1,2,3] -> [1,0,3,2]
        // then vcombine(high, low) swaps the halves: [1,0,3,2] -> [3,2,1,0]
        front = vrev64q_f32(front);
        front = vcombine_f32(vget_high_f32(front), vget_low_f32(front));

        back = vrev64q_f32(back);
        back = vcombine_f32(vget_high_f32(back), vget_low_f32(back));

        // Store crossed
        vst1q_f32(buf + lo, back);
        vst1q_f32(buf + hi, front);

        lo += 4;
        hi -= 4;
    }

    // Scalar remainder in the middle
    // After the NEON loop, lo and hi may overlap or have a small gap
    uint32_t slo = lo;
    uint32_t shi = (hi + 3 < len) ? hi + 3 : len - 1;

    while (slo < shi)
    {
        float tmp = buf[slo];
        buf[slo] = buf[shi];
        buf[shi] = tmp;
        slo++;
        shi--;
    }
}

// --- Float64 Reverse ---
inline void NeonReverseFloat64(double* buf, uint32_t len)
{
    if (len <= 1) return;

    uint32_t lo = 0;
    uint32_t hi = len - 2;

    while (lo + 1 < hi)
    {
        float64x2_t front = vld1q_f64(buf + lo);
        float64x2_t back  = vld1q_f64(buf + hi);

        // Reverse 2 doubles: swap lanes
        front = vcombine_f64(vget_high_f64(front), vget_low_f64(front));
        back  = vcombine_f64(vget_high_f64(back),  vget_low_f64(back));

        vst1q_f64(buf + lo, back);
        vst1q_f64(buf + hi, front);

        lo += 2;
        hi -= 2;
    }

    // Scalar middle
    uint32_t slo = lo;
    uint32_t shi = (hi + 1 < len) ? hi + 1 : len - 1;

    while (slo < shi)
    {
        double tmp = buf[slo];
        buf[slo] = buf[shi];
        buf[shi] = tmp;
        slo++;
        shi--;
    }
}

// --- Int32/Uint32 Reverse (shared via reinterpret) ---
inline void NeonReverseInt32(int32_t* buf, uint32_t len)
{
    // Reuse float32 reverse — same 4-byte element size, bit-exact swap
    NeonReverseFloat32(reinterpret_cast<float*>(buf), len);
}

inline void NeonReverseUint32(uint32_t* buf, uint32_t len)
{
    NeonReverseFloat32(reinterpret_cast<float*>(buf), len);
}

// --- Int16/Uint16 Reverse ---
inline void NeonReverseInt16(int16_t* buf, uint32_t len)
{
    if (len <= 1) return;

    uint32_t lo = 0;
    uint32_t hi = len - 8;

    while (lo + 7 < hi)
    {
        int16x8_t front = vld1q_s16(buf + lo);
        int16x8_t back  = vld1q_s16(buf + hi);

        // Reverse 8 int16s:
        // rev64 reverses within 64-bit halves: [0,1,2,3,4,5,6,7] -> [3,2,1,0,7,6,5,4]
        // then swap halves: -> [7,6,5,4,3,2,1,0]
        front = vrev64q_s16(front);
        front = vcombine_s16(vget_high_s16(front), vget_low_s16(front));

        back = vrev64q_s16(back);
        back = vcombine_s16(vget_high_s16(back), vget_low_s16(back));

        vst1q_s16(buf + lo, back);
        vst1q_s16(buf + hi, front);

        lo += 8;
        hi -= 8;
    }

    // Scalar middle
    uint32_t slo = lo;
    uint32_t shi = (hi + 7 < len) ? hi + 7 : len - 1;

    while (slo < shi)
    {
        int16_t tmp = buf[slo];
        buf[slo] = buf[shi];
        buf[shi] = tmp;
        slo++;
        shi--;
    }
}

inline void NeonReverseUint16(uint16_t* buf, uint32_t len)
{
    NeonReverseInt16(reinterpret_cast<int16_t*>(buf), len);
}

// --- Int8/Uint8 Reverse ---
inline void NeonReverseInt8(int8_t* buf, uint32_t len)
{
    if (len <= 1) return;

    uint32_t lo = 0;
    uint32_t hi = len - 16;

    // Prepare a TBL index vector to reverse 16 bytes: [15,14,13,...,1,0]
    static const uint8_t revIdx[16] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
    uint8x16_t tblIdx = vld1q_u8(revIdx);

    while (lo + 15 < hi)
    {
        int8x16_t front = vld1q_s8(buf + lo);
        int8x16_t back  = vld1q_s8(buf + hi);

        // Reverse 16 bytes using TBL
        uint8x16_t frontRev = vqtbl1q_u8(vreinterpretq_u8_s8(front), tblIdx);
        uint8x16_t backRev  = vqtbl1q_u8(vreinterpretq_u8_s8(back),  tblIdx);

        vst1q_s8(buf + lo, vreinterpretq_s8_u8(backRev));
        vst1q_s8(buf + hi, vreinterpretq_s8_u8(frontRev));

        lo += 16;
        hi -= 16;
    }

    // Scalar middle
    uint32_t slo = lo;
    uint32_t shi = (hi + 15 < len) ? hi + 15 : len - 1;

    while (slo < shi)
    {
        int8_t tmp = buf[slo];
        buf[slo] = buf[shi];
        buf[shi] = tmp;
        slo++;
        shi--;
    }
}

inline void NeonReverseUint8(uint8_t* buf, uint32_t len)
{
    NeonReverseInt8(reinterpret_cast<int8_t*>(buf), len);
}


// =====================================================================================
// Section 5: Bulk Copy Helpers
//
// NEON-accelerated typed copy for non-overlapping buffers.
// For same-type copies, this is essentially a NEON memcpy.
// For overlapping regions (when src and dst are in the same buffer),
// use NeonCopyBackward* variants that copy from the end.
// =====================================================================================

// --- Float32 Copy (forward, non-overlapping) ---
inline void NeonCopyFloat32(float* dst, const float* src, uint32_t count)
{
    uint32_t i = 0;

    for (; i + 15 < count; i += 16)
    {
        float32x4_t v0 = vld1q_f32(src + i);
        float32x4_t v1 = vld1q_f32(src + i + 4);
        float32x4_t v2 = vld1q_f32(src + i + 8);
        float32x4_t v3 = vld1q_f32(src + i + 12);
        vst1q_f32(dst + i,      v0);
        vst1q_f32(dst + i + 4,  v1);
        vst1q_f32(dst + i + 8,  v2);
        vst1q_f32(dst + i + 12, v3);
    }
    for (; i + 3 < count; i += 4)
    {
        float32x4_t v = vld1q_f32(src + i);
        vst1q_f32(dst + i, v);
    }
    for (; i < count; i++)
    {
        dst[i] = src[i];
    }
}

// --- Float64 Copy ---
inline void NeonCopyFloat64(double* dst, const double* src, uint32_t count)
{
    uint32_t i = 0;

    for (; i + 7 < count; i += 8)
    {
        float64x2_t v0 = vld1q_f64(src + i);
        float64x2_t v1 = vld1q_f64(src + i + 2);
        float64x2_t v2 = vld1q_f64(src + i + 4);
        float64x2_t v3 = vld1q_f64(src + i + 6);
        vst1q_f64(dst + i,     v0);
        vst1q_f64(dst + i + 2, v1);
        vst1q_f64(dst + i + 4, v2);
        vst1q_f64(dst + i + 6, v3);
    }
    for (; i + 1 < count; i += 2)
    {
        float64x2_t v = vld1q_f64(src + i);
        vst1q_f64(dst + i, v);
    }
    for (; i < count; i++)
    {
        dst[i] = src[i];
    }
}

// --- Generic 128-bit aligned copy (for int32/uint32 arrays) ---
inline void NeonCopyInt32(int32_t* dst, const int32_t* src, uint32_t count)
{
    uint32_t i = 0;

    for (; i + 15 < count; i += 16)
    {
        int32x4_t v0 = vld1q_s32(src + i);
        int32x4_t v1 = vld1q_s32(src + i + 4);
        int32x4_t v2 = vld1q_s32(src + i + 8);
        int32x4_t v3 = vld1q_s32(src + i + 12);
        vst1q_s32(dst + i,      v0);
        vst1q_s32(dst + i + 4,  v1);
        vst1q_s32(dst + i + 8,  v2);
        vst1q_s32(dst + i + 12, v3);
    }
    for (; i + 3 < count; i += 4)
    {
        int32x4_t v = vld1q_s32(src + i);
        vst1q_s32(dst + i, v);
    }
    for (; i < count; i++)
    {
        dst[i] = src[i];
    }
}

// --- Int8 copy ---
inline void NeonCopyInt8(int8_t* dst, const int8_t* src, uint32_t count)
{
    uint32_t i = 0;

    for (; i + 63 < count; i += 64)
    {
        int8x16_t v0 = vld1q_s8(src + i);
        int8x16_t v1 = vld1q_s8(src + i + 16);
        int8x16_t v2 = vld1q_s8(src + i + 32);
        int8x16_t v3 = vld1q_s8(src + i + 48);
        vst1q_s8(dst + i,      v0);
        vst1q_s8(dst + i + 16, v1);
        vst1q_s8(dst + i + 32, v2);
        vst1q_s8(dst + i + 48, v3);
    }
    for (; i + 15 < count; i += 16)
    {
        int8x16_t v = vld1q_s8(src + i);
        vst1q_s8(dst + i, v);
    }
    for (; i < count; i++)
    {
        dst[i] = src[i];
    }
}

// --- Int16 copy ---
inline void NeonCopyInt16(int16_t* dst, const int16_t* src, uint32_t count)
{
    uint32_t i = 0;

    for (; i + 31 < count; i += 32)
    {
        int16x8_t v0 = vld1q_s16(src + i);
        int16x8_t v1 = vld1q_s16(src + i + 8);
        int16x8_t v2 = vld1q_s16(src + i + 16);
        int16x8_t v3 = vld1q_s16(src + i + 24);
        vst1q_s16(dst + i,      v0);
        vst1q_s16(dst + i + 8,  v1);
        vst1q_s16(dst + i + 16, v2);
        vst1q_s16(dst + i + 24, v3);
    }
    for (; i + 7 < count; i += 8)
    {
        int16x8_t v = vld1q_s16(src + i);
        vst1q_s16(dst + i, v);
    }
    for (; i < count; i++)
    {
        dst[i] = src[i];
    }
}


// =====================================================================================
// Section 6: SIMD Operation Helpers
//
// Wrappers for common SIMD patterns used by SimdXxxOperation.cpp files.
// These take raw SIMDValue-compatible arrays (float[4], int32[4], etc.)
// and perform vector operations in a single NEON instruction.
// =====================================================================================

// --- Float32x4 helpers (4-lane float) ---

inline void NeonSimdFloat32x4Add(const float* a, const float* b, float* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_f32(result, vaddq_f32(va, vb));
}

inline void NeonSimdFloat32x4Sub(const float* a, const float* b, float* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_f32(result, vsubq_f32(va, vb));
}

inline void NeonSimdFloat32x4Mul(const float* a, const float* b, float* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_f32(result, vmulq_f32(va, vb));
}

inline void NeonSimdFloat32x4Div(const float* a, const float* b, float* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_f32(result, vdivq_f32(va, vb));
}

inline void NeonSimdFloat32x4Abs(const float* a, float* result)
{
    float32x4_t va = vld1q_f32(a);
    vst1q_f32(result, vabsq_f32(va));
}

inline void NeonSimdFloat32x4Neg(const float* a, float* result)
{
    float32x4_t va = vld1q_f32(a);
    vst1q_f32(result, vnegq_f32(va));
}

inline void NeonSimdFloat32x4Sqrt(const float* a, float* result)
{
    float32x4_t va = vld1q_f32(a);
    vst1q_f32(result, vsqrtq_f32(va));
}

inline void NeonSimdFloat32x4Reciprocal(const float* a, float* result)
{
    float32x4_t va = vld1q_f32(a);
    // Full precision reciprocal via vdivq (not vrecpeq which is an estimate)
    float32x4_t ones = vdupq_n_f32(1.0f);
    vst1q_f32(result, vdivq_f32(ones, va));
}

inline void NeonSimdFloat32x4ReciprocalSqrt(const float* a, float* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t ones = vdupq_n_f32(1.0f);
    vst1q_f32(result, vsqrtq_f32(vdivq_f32(ones, va)));
}

inline void NeonSimdFloat32x4Scale(const float* a, float scaleValue, float* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vs = vdupq_n_f32(scaleValue);
    vst1q_f32(result, vmulq_f32(va, vs));
}

inline void NeonSimdFloat32x4Splat(float x, float* result)
{
    float32x4_t v = vdupq_n_f32(x);
    vst1q_f32(result, v);
}

// Comparison helpers return int32 masks: 0xFFFFFFFF for true, 0x00000000 for false.
inline void NeonSimdFloat32x4CmpEq(const float* a, const float* b, uint32_t* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_u32(result, vceqq_f32(va, vb));
}

inline void NeonSimdFloat32x4CmpLt(const float* a, const float* b, uint32_t* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_u32(result, vcltq_f32(va, vb));
}

inline void NeonSimdFloat32x4CmpLe(const float* a, const float* b, uint32_t* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_u32(result, vcleq_f32(va, vb));
}

inline void NeonSimdFloat32x4CmpGt(const float* a, const float* b, uint32_t* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_u32(result, vcgtq_f32(va, vb));
}

inline void NeonSimdFloat32x4CmpGe(const float* a, const float* b, uint32_t* result)
{
    float32x4_t va = vld1q_f32(a);
    float32x4_t vb = vld1q_f32(b);
    vst1q_u32(result, vcgeq_f32(va, vb));
}

// --- Float64x2 helpers (2-lane double) ---

inline void NeonSimdFloat64x2Add(const double* a, const double* b, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vb = vld1q_f64(b);
    vst1q_f64(result, vaddq_f64(va, vb));
}

inline void NeonSimdFloat64x2Sub(const double* a, const double* b, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vb = vld1q_f64(b);
    vst1q_f64(result, vsubq_f64(va, vb));
}

inline void NeonSimdFloat64x2Mul(const double* a, const double* b, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vb = vld1q_f64(b);
    vst1q_f64(result, vmulq_f64(va, vb));
}

inline void NeonSimdFloat64x2Div(const double* a, const double* b, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vb = vld1q_f64(b);
    vst1q_f64(result, vdivq_f64(va, vb));
}

inline void NeonSimdFloat64x2Abs(const double* a, double* result)
{
    float64x2_t va = vld1q_f64(a);
    vst1q_f64(result, vabsq_f64(va));
}

inline void NeonSimdFloat64x2Neg(const double* a, double* result)
{
    float64x2_t va = vld1q_f64(a);
    vst1q_f64(result, vnegq_f64(va));
}

inline void NeonSimdFloat64x2Sqrt(const double* a, double* result)
{
    float64x2_t va = vld1q_f64(a);
    vst1q_f64(result, vsqrtq_f64(va));
}

inline void NeonSimdFloat64x2Reciprocal(const double* a, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t ones = vdupq_n_f64(1.0);
    vst1q_f64(result, vdivq_f64(ones, va));
}

inline void NeonSimdFloat64x2ReciprocalSqrt(const double* a, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t ones = vdupq_n_f64(1.0);
    vst1q_f64(result, vsqrtq_f64(vdivq_f64(ones, va)));
}

inline void NeonSimdFloat64x2Scale(const double* a, double scaleValue, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vs = vdupq_n_f64(scaleValue);
    vst1q_f64(result, vmulq_f64(va, vs));
}

inline void NeonSimdFloat64x2Min(const double* a, const double* b, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vb = vld1q_f64(b);
    vst1q_f64(result, vminq_f64(va, vb));
}

inline void NeonSimdFloat64x2Max(const double* a, const double* b, double* result)
{
    float64x2_t va = vld1q_f64(a);
    float64x2_t vb = vld1q_f64(b);
    vst1q_f64(result, vmaxq_f64(va, vb));
}

// --- Int32x4 helpers (4-lane int32) ---

inline void NeonSimdInt32x4Add(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vaddq_s32(va, vb));
}

inline void NeonSimdInt32x4Sub(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vsubq_s32(va, vb));
}

inline void NeonSimdInt32x4Mul(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vmulq_s32(va, vb));
}

inline void NeonSimdInt32x4Abs(const int32_t* a, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    vst1q_s32(result, vabsq_s32(va));
}

inline void NeonSimdInt32x4Neg(const int32_t* a, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    vst1q_s32(result, vnegq_s32(va));
}

inline void NeonSimdInt32x4Not(const int32_t* a, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    vst1q_s32(result, vmvnq_s32(va));
}

inline void NeonSimdInt32x4And(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vandq_s32(va, vb));
}

inline void NeonSimdInt32x4Or(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vorrq_s32(va, vb));
}

inline void NeonSimdInt32x4Xor(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, veorq_s32(va, vb));
}

inline void NeonSimdInt32x4Min(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vminq_s32(va, vb));
}

inline void NeonSimdInt32x4Max(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    vst1q_s32(result, vmaxq_s32(va, vb));
}

inline void NeonSimdInt32x4CmpEq(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    uint32x4_t cmp = vceqq_s32(va, vb);
    vst1q_s32(result, vreinterpretq_s32_u32(cmp));
}

inline void NeonSimdInt32x4CmpLt(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    uint32x4_t cmp = vcltq_s32(va, vb);
    vst1q_s32(result, vreinterpretq_s32_u32(cmp));
}

inline void NeonSimdInt32x4CmpLe(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    uint32x4_t cmp = vcleq_s32(va, vb);
    vst1q_s32(result, vreinterpretq_s32_u32(cmp));
}

inline void NeonSimdInt32x4CmpGt(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    uint32x4_t cmp = vcgtq_s32(va, vb);
    vst1q_s32(result, vreinterpretq_s32_u32(cmp));
}

inline void NeonSimdInt32x4CmpGe(const int32_t* a, const int32_t* b, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    int32x4_t vb = vld1q_s32(b);
    uint32x4_t cmp = vcgeq_s32(va, vb);
    vst1q_s32(result, vreinterpretq_s32_u32(cmp));
}

inline void NeonSimdInt32x4ShiftLeft(const int32_t* a, int count, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    // vshlq takes a signed shift amount per lane; positive = left shift
    int32x4_t vcount = vdupq_n_s32(count);
    vst1q_s32(result, vshlq_s32(va, vcount));
}

inline void NeonSimdInt32x4ShiftRight(const int32_t* a, int count, int32_t* result)
{
    int32x4_t va = vld1q_s32(a);
    // Negative shift amount = right shift in vshlq
    int32x4_t vcount = vdupq_n_s32(-count);
    vst1q_s32(result, vshlq_s32(va, vcount));
}

inline void NeonSimdInt32x4Splat(int32_t x, int32_t* result)
{
    int32x4_t v = vdupq_n_s32(x);
    vst1q_s32(result, v);
}

// Select: result = bitselect(mask, trueVal, falseVal)
// For each bit: if mask bit is 1, take from trueVal; else take from falseVal.
inline void NeonSimdInt32x4Select(const int32_t* mask, const int32_t* trueVal, const int32_t* falseVal, int32_t* result)
{
    uint32x4_t vm = vld1q_u32(reinterpret_cast<const uint32_t*>(mask));
    uint32x4_t vt = vld1q_u32(reinterpret_cast<const uint32_t*>(trueVal));
    uint32x4_t vf = vld1q_u32(reinterpret_cast<const uint32_t*>(falseVal));
    vst1q_u32(reinterpret_cast<uint32_t*>(result), vbslq_u32(vm, vt, vf));
}

// --- Uint32x4 helpers ---

inline void NeonSimdUint32x4Min(const uint32_t* a, const uint32_t* b, uint32_t* result)
{
    uint32x4_t va = vld1q_u32(a);
    uint32x4_t vb = vld1q_u32(b);
    vst1q_u32(result, vminq_u32(va, vb));
}

inline void NeonSimdUint32x4Max(const uint32_t* a, const uint32_t* b, uint32_t* result)
{
    uint32x4_t va = vld1q_u32(a);
    uint32x4_t vb = vld1q_u32(b);
    vst1q_u32(result, vmaxq_u32(va, vb));
}

inline void NeonSimdUint32x4CmpLt(const uint32_t* a, const uint32_t* b, uint32_t* result)
{
    uint32x4_t va = vld1q_u32(a);
    uint32x4_t vb = vld1q_u32(b);
    vst1q_u32(result, vcltq_u32(va, vb));
}

inline void NeonSimdUint32x4CmpLe(const uint32_t* a, const uint32_t* b, uint32_t* result)
{
    uint32x4_t va = vld1q_u32(a);
    uint32x4_t vb = vld1q_u32(b);
    vst1q_u32(result, vcleq_u32(va, vb));
}

inline void NeonSimdUint32x4ShiftRight(const uint32_t* a, int count, uint32_t* result)
{
    uint32x4_t va = vld1q_u32(a);
    int32x4_t vcount = vdupq_n_s32(-count);
    vst1q_u32(result, vshlq_u32(va, vcount));
}

// --- Int16x8 helpers (8-lane int16) ---

inline void NeonSimdInt16x8Add(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vaddq_s16(va, vb));
}

inline void NeonSimdInt16x8Sub(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vsubq_s16(va, vb));
}

inline void NeonSimdInt16x8Mul(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vmulq_s16(va, vb));
}

inline void NeonSimdInt16x8Neg(const int16_t* a, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    vst1q_s16(result, vnegq_s16(va));
}

inline void NeonSimdInt16x8Not(const int16_t* a, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    vst1q_s16(result, vmvnq_s16(va));
}

inline void NeonSimdInt16x8And(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vandq_s16(va, vb));
}

inline void NeonSimdInt16x8Or(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vorrq_s16(va, vb));
}

inline void NeonSimdInt16x8Xor(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, veorq_s16(va, vb));
}

inline void NeonSimdInt16x8Min(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vminq_s16(va, vb));
}

inline void NeonSimdInt16x8Max(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vmaxq_s16(va, vb));
}

inline void NeonSimdInt16x8AddSaturate(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vqaddq_s16(va, vb));
}

inline void NeonSimdInt16x8SubSaturate(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vqsubq_s16(va, vb));
}

inline void NeonSimdInt16x8CmpEq(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vreinterpretq_s16_u16(vceqq_s16(va, vb)));
}

inline void NeonSimdInt16x8CmpLt(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vreinterpretq_s16_u16(vcltq_s16(va, vb)));
}

inline void NeonSimdInt16x8CmpLe(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vreinterpretq_s16_u16(vcleq_s16(va, vb)));
}

inline void NeonSimdInt16x8CmpGt(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vreinterpretq_s16_u16(vcgtq_s16(va, vb)));
}

inline void NeonSimdInt16x8CmpGe(const int16_t* a, const int16_t* b, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vb = vld1q_s16(b);
    vst1q_s16(result, vreinterpretq_s16_u16(vcgeq_s16(va, vb)));
}

inline void NeonSimdInt16x8ShiftLeft(const int16_t* a, int count, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vcount = vdupq_n_s16((int16_t)count);
    vst1q_s16(result, vshlq_s16(va, vcount));
}

inline void NeonSimdInt16x8ShiftRight(const int16_t* a, int count, int16_t* result)
{
    int16x8_t va = vld1q_s16(a);
    int16x8_t vcount = vdupq_n_s16((int16_t)(-count));
    vst1q_s16(result, vshlq_s16(va, vcount));
}

inline void NeonSimdInt16x8Splat(int16_t x, int16_t* result)
{
    int16x8_t v = vdupq_n_s16(x);
    vst1q_s16(result, v);
}

// --- Int8x16 helpers (16-lane int8) ---

inline void NeonSimdInt8x16Add(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vaddq_s8(va, vb));
}

inline void NeonSimdInt8x16Sub(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vsubq_s8(va, vb));
}

inline void NeonSimdInt8x16Mul(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vmulq_s8(va, vb));
}

inline void NeonSimdInt8x16Neg(const int8_t* a, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    vst1q_s8(result, vnegq_s8(va));
}

inline void NeonSimdInt8x16AddSaturate(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vqaddq_s8(va, vb));
}

inline void NeonSimdInt8x16SubSaturate(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vqsubq_s8(va, vb));
}

inline void NeonSimdInt8x16Min(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vminq_s8(va, vb));
}

inline void NeonSimdInt8x16Max(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vmaxq_s8(va, vb));
}

inline void NeonSimdInt8x16CmpEq(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vreinterpretq_s8_u8(vceqq_s8(va, vb)));
}

inline void NeonSimdInt8x16CmpLt(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vreinterpretq_s8_u8(vcltq_s8(va, vb)));
}

inline void NeonSimdInt8x16CmpLe(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vreinterpretq_s8_u8(vcleq_s8(va, vb)));
}

inline void NeonSimdInt8x16CmpGt(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vreinterpretq_s8_u8(vcgtq_s8(va, vb)));
}

inline void NeonSimdInt8x16CmpGe(const int8_t* a, const int8_t* b, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vb = vld1q_s8(b);
    vst1q_s8(result, vreinterpretq_s8_u8(vcgeq_s8(va, vb)));
}

inline void NeonSimdInt8x16ShiftLeft(const int8_t* a, int count, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vcount = vdupq_n_s8((int8_t)count);
    vst1q_s8(result, vshlq_s8(va, vcount));
}

inline void NeonSimdInt8x16ShiftRight(const int8_t* a, int count, int8_t* result)
{
    int8x16_t va = vld1q_s8(a);
    int8x16_t vcount = vdupq_n_s8((int8_t)(-count));
    vst1q_s8(result, vshlq_s8(va, vcount));
}

inline void NeonSimdInt8x16Splat(int8_t x, int8_t* result)
{
    int8x16_t v = vdupq_n_s8(x);
    vst1q_s8(result, v);
}

// --- Uint16x8 helpers ---

inline void NeonSimdUint16x8AddSaturate(const uint16_t* a, const uint16_t* b, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    uint16x8_t vb = vld1q_u16(b);
    vst1q_u16(result, vqaddq_u16(va, vb));
}

inline void NeonSimdUint16x8SubSaturate(const uint16_t* a, const uint16_t* b, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    uint16x8_t vb = vld1q_u16(b);
    vst1q_u16(result, vqsubq_u16(va, vb));
}

inline void NeonSimdUint16x8Min(const uint16_t* a, const uint16_t* b, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    uint16x8_t vb = vld1q_u16(b);
    vst1q_u16(result, vminq_u16(va, vb));
}

inline void NeonSimdUint16x8Max(const uint16_t* a, const uint16_t* b, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    uint16x8_t vb = vld1q_u16(b);
    vst1q_u16(result, vmaxq_u16(va, vb));
}

inline void NeonSimdUint16x8ShiftRight(const uint16_t* a, int count, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    int16x8_t vcount = vdupq_n_s16((int16_t)(-count));
    vst1q_u16(result, vshlq_u16(va, vcount));
}

inline void NeonSimdUint16x8CmpLt(const uint16_t* a, const uint16_t* b, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    uint16x8_t vb = vld1q_u16(b);
    vst1q_u16(result, vcltq_u16(va, vb));
}

inline void NeonSimdUint16x8CmpLe(const uint16_t* a, const uint16_t* b, uint16_t* result)
{
    uint16x8_t va = vld1q_u16(a);
    uint16x8_t vb = vld1q_u16(b);
    vst1q_u16(result, vcleq_u16(va, vb));
}

// --- Uint8x16 helpers ---

inline void NeonSimdUint8x16AddSaturate(const uint8_t* a, const uint8_t* b, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    uint8x16_t vb = vld1q_u8(b);
    vst1q_u8(result, vqaddq_u8(va, vb));
}

inline void NeonSimdUint8x16SubSaturate(const uint8_t* a, const uint8_t* b, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    uint8x16_t vb = vld1q_u8(b);
    vst1q_u8(result, vqsubq_u8(va, vb));
}

inline void NeonSimdUint8x16Min(const uint8_t* a, const uint8_t* b, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    uint8x16_t vb = vld1q_u8(b);
    vst1q_u8(result, vminq_u8(va, vb));
}

inline void NeonSimdUint8x16Max(const uint8_t* a, const uint8_t* b, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    uint8x16_t vb = vld1q_u8(b);
    vst1q_u8(result, vmaxq_u8(va, vb));
}

inline void NeonSimdUint8x16ShiftRight(const uint8_t* a, int count, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    int8x16_t vcount = vdupq_n_s8((int8_t)(-count));
    vst1q_u8(result, vshlq_u8(va, vcount));
}

inline void NeonSimdUint8x16CmpLt(const uint8_t* a, const uint8_t* b, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    uint8x16_t vb = vld1q_u8(b);
    vst1q_u8(result, vcltq_u8(va, vb));
}

inline void NeonSimdUint8x16CmpLe(const uint8_t* a, const uint8_t* b, uint8_t* result)
{
    uint8x16_t va = vld1q_u8(a);
    uint8x16_t vb = vld1q_u8(b);
    vst1q_u8(result, vcleq_u8(va, vb));
}

// --- Int64x2 helpers (2-lane int64) ---

inline void NeonSimdInt64x2Add(const int64_t* a, const int64_t* b, int64_t* result)
{
    int64x2_t va = vld1q_s64(a);
    int64x2_t vb = vld1q_s64(b);
    vst1q_s64(result, vaddq_s64(va, vb));
}

inline void NeonSimdInt64x2Sub(const int64_t* a, const int64_t* b, int64_t* result)
{
    int64x2_t va = vld1q_s64(a);
    int64x2_t vb = vld1q_s64(b);
    vst1q_s64(result, vsubq_s64(va, vb));
}

inline void NeonSimdInt64x2Neg(const int64_t* a, int64_t* result)
{
    int64x2_t va = vld1q_s64(a);
    vst1q_s64(result, vnegq_s64(va));
}

inline void NeonSimdInt64x2Splat(int64_t x, int64_t* result)
{
    int64x2_t v = vdupq_n_s64(x);
    vst1q_s64(result, v);
}

inline void NeonSimdInt64x2ShiftLeft(const int64_t* a, int count, int64_t* result)
{
    int64x2_t va = vld1q_s64(a);
    int64x2_t vcount = vdupq_n_s64((int64_t)count);
    vst1q_s64(result, vshlq_s64(va, vcount));
}

inline void NeonSimdInt64x2ShiftRight(const int64_t* a, int count, int64_t* result)
{
    int64x2_t va = vld1q_s64(a);
    int64x2_t vcount = vdupq_n_s64((int64_t)(-count));
    vst1q_s64(result, vshlq_s64(va, vcount));
}

inline void NeonSimdUint64x2ShiftRight(const int64_t* a, int count, int64_t* result)
{
    uint64x2_t va = vld1q_u64(reinterpret_cast<const uint64_t*>(a));
    int64x2_t vcount = vdupq_n_s64((int64_t)(-count));
    vst1q_u64(reinterpret_cast<uint64_t*>(result), vshlq_u64(va, vcount));
}

// --- Conversion helpers ---

inline void NeonSimdConvertInt32x4ToFloat32x4(const int32_t* src, float* dst)
{
    int32x4_t vi = vld1q_s32(src);
    vst1q_f32(dst, vcvtq_f32_s32(vi));
}

inline void NeonSimdConvertUint32x4ToFloat32x4(const uint32_t* src, float* dst)
{
    uint32x4_t vi = vld1q_u32(src);
    vst1q_f32(dst, vcvtq_f32_u32(vi));
}

inline void NeonSimdConvertFloat32x4ToInt32x4(const float* src, int32_t* dst)
{
    float32x4_t vf = vld1q_f32(src);
    vst1q_s32(dst, vcvtq_s32_f32(vf));
}

inline void NeonSimdConvertFloat32x4ToUint32x4(const float* src, uint32_t* dst)
{
    float32x4_t vf = vld1q_f32(src);
    vst1q_u32(dst, vcvtq_u32_f32(vf));
}

inline void NeonSimdConvertFloat32x4ToFloat64x2(const float* src, double* dst)
{
    // Convert only the low two float32 lanes to float64
    float32x2_t lo = vld1_f32(src);
    vst1q_f64(dst, vcvt_f64_f32(lo));
}

inline void NeonSimdConvertInt32x4ToFloat64x2(const int32_t* src, double* dst)
{
    // Convert only the low two int32 lanes to float64
    int32x2_t lo = vld1_s32(src);
    float32x2_t flo = vcvt_f32_s32(lo);
    vst1q_f64(dst, vcvt_f64_f32(flo));
}


// =====================================================================================
// Section 7: Bool operation helpers
//
// For SIMDBool32x4/16x8/8x16 AnyTrue/AllTrue operations.
// =====================================================================================

// Check if any 32-bit lane is non-zero
inline bool NeonSimdBool32x4AnyTrue(const int32_t* v)
{
    int32x4_t va = vld1q_s32(v);
    // OR all lanes together — if any is non-zero, result is non-zero
    uint32x2_t reduced = vorr_u32(
        vreinterpret_u32_s32(vget_low_s32(va)),
        vreinterpret_u32_s32(vget_high_s32(va)));
    return (vget_lane_u32(vpmax_u32(reduced, reduced), 0) != 0);
}

// Check if all 32-bit lanes are non-zero
inline bool NeonSimdBool32x4AllTrue(const int32_t* v)
{
    int32x4_t va = vld1q_s32(v);
    // AND all lanes — result is non-zero only if all are non-zero
    uint32x2_t reduced = vand_u32(
        vreinterpret_u32_s32(vget_low_s32(va)),
        vreinterpret_u32_s32(vget_high_s32(va)));
    return (vget_lane_u32(vpmin_u32(reduced, reduced), 0) != 0);
}

// Check if any byte lane is non-zero (for Bool8x16/Bool16x8 after canonicalization to 32-bit)
inline bool NeonSimdAnyLaneNonZero16B(const int32_t* v)
{
    uint8x16_t va = vld1q_u8(reinterpret_cast<const uint8_t*>(v));
    return (vmaxvq_u8(va) != 0);
}

inline bool NeonSimdAllLanesNonZero4S(const int32_t* v)
{
    // For canonicalized bool values (0 or -1 in each i32 lane)
    uint32x4_t va = vld1q_u32(reinterpret_cast<const uint32_t*>(v));
    return (vminvq_u32(va) != 0);
}

} // namespace NeonAccel

#endif // CHAKRA_NEON_AVAILABLE