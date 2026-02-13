//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDFloat32x4Operation::OpFloat32x4(float x, float y, float z, float w)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float vals[4] = { x, y, z, w };
        float32x4_t v = vld1q_f32(vals);
        vst1q_f32(result.f32, v);
#else
        result.f32[SIMD_X] = x;
        result.f32[SIMD_Y] = y;
        result.f32[SIMD_Z] = z;
        result.f32[SIMD_W] = w;
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpSplat(float x)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        vst1q_f32(result.f32, vdupq_n_f32(x));
#else
        result.f32[SIMD_X] = result.f32[SIMD_Y] = result.f32[SIMD_Z] = result.f32[SIMD_W] = x;
#endif
        return result;
    }

    // Conversions
    SIMDValue SIMDFloat32x4Operation::OpFromFloat64x2(const SIMDValue& v)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        // Convert the two float64 lanes to float32
        float32x2_t lo = vcvt_f32_f64(vld1q_f64(v.f64));
        // Zero the high two lanes
        float32x4_t full = vcombine_f32(lo, vdup_n_f32(0.0f));
        vst1q_f32(result.f32, full);
#else
        result.f32[SIMD_X] = (float)(v.f64[SIMD_X]);
        result.f32[SIMD_Y] = (float)(v.f64[SIMD_Y]);
        result.f32[SIMD_Z] = result.f32[SIMD_W] = 0;
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpFromInt32x4(const SIMDValue& v)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t vi = vld1q_s32(v.i32);
        vst1q_f32(result.f32, vcvtq_f32_s32(vi));
#else
        result.f32[SIMD_X] = (float)(v.i32[SIMD_X]);
        result.f32[SIMD_Y] = (float)(v.i32[SIMD_Y]);
        result.f32[SIMD_Z] = (float)(v.i32[SIMD_Z]);
        result.f32[SIMD_W] = (float)(v.i32[SIMD_W]);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpFromUint32x4(const SIMDValue& v)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        uint32x4_t vu = vld1q_u32(v.u32);
        vst1q_f32(result.f32, vcvtq_f32_u32(vu));
#else
        result.f32[SIMD_X] = (float)(v.u32[SIMD_X]);
        result.f32[SIMD_Y] = (float)(v.u32[SIMD_Y]);
        result.f32[SIMD_Z] = (float)(v.u32[SIMD_Z]);
        result.f32[SIMD_W] = (float)(v.u32[SIMD_W]);
#endif
        return result;
    }

    // Unary Ops
    SIMDValue SIMDFloat32x4Operation::OpAbs(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(value.f32);
        vst1q_f32(result.f32, vabsq_f32(va));
#else
        result.f32[SIMD_X] = (value.f32[SIMD_X] < 0) ? -1 * value.f32[SIMD_X] : value.f32[SIMD_X];
        result.f32[SIMD_Y] = (value.f32[SIMD_Y] < 0) ? -1 * value.f32[SIMD_Y] : value.f32[SIMD_Y];
        result.f32[SIMD_Z] = (value.f32[SIMD_Z] < 0) ? -1 * value.f32[SIMD_Z] : value.f32[SIMD_Z];
        result.f32[SIMD_W] = (value.f32[SIMD_W] < 0) ? -1 * value.f32[SIMD_W] : value.f32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(value.f32);
        vst1q_f32(result.f32, vnegq_f32(va));
#else
        result.f32[SIMD_X] = -1 * value.f32[SIMD_X];
        result.f32[SIMD_Y] = -1 * value.f32[SIMD_Y];
        result.f32[SIMD_Z] = -1 * value.f32[SIMD_Z];
        result.f32[SIMD_W] = -1 * value.f32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpNot(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        // Bitwise NOT via integer reinterpret
        int32x4_t vi = vld1q_s32(value.i32);
        vst1q_s32(result.i32, vmvnq_s32(vi));
#else
        result = SIMDInt32x4Operation::OpNot(value);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpReciprocal(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(value.f32);
        float32x4_t ones = vdupq_n_f32(1.0f);
        vst1q_f32(result.f32, vdivq_f32(ones, va));
#else
        result.f32[SIMD_X] = (float)(1.0 / (value.f32[SIMD_X]));
        result.f32[SIMD_Y] = (float)(1.0 / (value.f32[SIMD_Y]));
        result.f32[SIMD_Z] = (float)(1.0 / (value.f32[SIMD_Z]));
        result.f32[SIMD_W] = (float)(1.0 / (value.f32[SIMD_W]));
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpReciprocalSqrt(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(value.f32);
        float32x4_t ones = vdupq_n_f32(1.0f);
        vst1q_f32(result.f32, vsqrtq_f32(vdivq_f32(ones, va)));
#else
        result.f32[SIMD_X] = (float)sqrt(1.0 / (value.f32[SIMD_X]));
        result.f32[SIMD_Y] = (float)sqrt(1.0 / (value.f32[SIMD_Y]));
        result.f32[SIMD_Z] = (float)sqrt(1.0 / (value.f32[SIMD_Z]));
        result.f32[SIMD_W] = (float)sqrt(1.0 / (value.f32[SIMD_W]));
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpSqrt(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(value.f32);
        vst1q_f32(result.f32, vsqrtq_f32(va));
#else
        result.f32[SIMD_X] = sqrtf(value.f32[SIMD_X]);
        result.f32[SIMD_Y] = sqrtf(value.f32[SIMD_Y]);
        result.f32[SIMD_Z] = sqrtf(value.f32[SIMD_Z]);
        result.f32[SIMD_W] = sqrtf(value.f32[SIMD_W]);
#endif
        return result;
    }

    // Binary Ops
    SIMDValue SIMDFloat32x4Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        vst1q_f32(result.f32, vaddq_f32(va, vb));
#else
        result.f32[SIMD_X] = aValue.f32[SIMD_X] + bValue.f32[SIMD_X];
        result.f32[SIMD_Y] = aValue.f32[SIMD_Y] + bValue.f32[SIMD_Y];
        result.f32[SIMD_Z] = aValue.f32[SIMD_Z] + bValue.f32[SIMD_Z];
        result.f32[SIMD_W] = aValue.f32[SIMD_W] + bValue.f32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        vst1q_f32(result.f32, vsubq_f32(va, vb));
#else
        result.f32[SIMD_X] = aValue.f32[SIMD_X] - bValue.f32[SIMD_X];
        result.f32[SIMD_Y] = aValue.f32[SIMD_Y] - bValue.f32[SIMD_Y];
        result.f32[SIMD_Z] = aValue.f32[SIMD_Z] - bValue.f32[SIMD_Z];
        result.f32[SIMD_W] = aValue.f32[SIMD_W] - bValue.f32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        vst1q_f32(result.f32, vmulq_f32(va, vb));
#else
        result.f32[SIMD_X] = aValue.f32[SIMD_X] * bValue.f32[SIMD_X];
        result.f32[SIMD_Y] = aValue.f32[SIMD_Y] * bValue.f32[SIMD_Y];
        result.f32[SIMD_Z] = aValue.f32[SIMD_Z] * bValue.f32[SIMD_Z];
        result.f32[SIMD_W] = aValue.f32[SIMD_W] * bValue.f32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpDiv(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        vst1q_f32(result.f32, vdivq_f32(va, vb));
#else
        result.f32[SIMD_X] = aValue.f32[SIMD_X] / bValue.f32[SIMD_X];
        result.f32[SIMD_Y] = aValue.f32[SIMD_Y] / bValue.f32[SIMD_Y];
        result.f32[SIMD_Z] = aValue.f32[SIMD_Z] / bValue.f32[SIMD_Z];
        result.f32[SIMD_W] = aValue.f32[SIMD_W] / bValue.f32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vandq_s32(va, vb));
#else
        result = SIMDInt32x4Operation::OpAnd(aValue, bValue);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vorrq_s32(va, vb));
#else
        result = SIMDInt32x4Operation::OpOr(aValue, bValue);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, veorq_s32(va, vb));
#else
        result = SIMDInt32x4Operation::OpXor(aValue, bValue);
#endif
        return result;
    }

    /*
    Min/Max(a, b) spec semantics:
    If any value is NaN, return NaN
    a < b ? a : b; where +0.0 > -0.0 (vice versa for Max)

    NEON's FMIN/FMAX propagate NaN correctly (ARMv8 semantics).
    However, FMIN treats -0 and +0 as equal, so we need a scalar fixup
    for the -0/+0 edge case to match the JS SIMD spec.
    */
    SIMDValue SIMDFloat32x4Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        // Keep the scalar implementation for Min because of the specific
        // -0/+0 handling required by the JS spec that differs from NEON FMIN behavior.
        // The per-lane overhead is acceptable for 4 elements.
        for (uint i = 0; i < 4; i++)
        {
            float a = aValue.f32[i];
            float b = bValue.f32[i];
            if (Js::NumberUtilities::IsNan(a))
            {
                result.f32[i] = a;
            }
            else if (Js::NumberUtilities::IsNan(b))
            {
                result.f32[i] = b;
            }
            else if (Js::NumberUtilities::IsFloat32NegZero(a) && b >= 0.0)
            {
                result.f32[i] = a;
            }
            else if (Js::NumberUtilities::IsFloat32NegZero(b) && a >= 0.0)
            {
                result.f32[i] = b;
            }
            else
            {
                result.f32[i] = a < b ? a : b;
            }
        }
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        // Keep the scalar implementation for Max for the same -0/+0 reason as OpMin.
        for (uint i = 0; i < 4; i++)
        {
            float a = aValue.f32[i];
            float b = bValue.f32[i];
            if (Js::NumberUtilities::IsNan(a))
            {
                result.f32[i] = a;
            }
            else if (Js::NumberUtilities::IsNan(b))
            {
                result.f32[i] = b;
            }
            else if (Js::NumberUtilities::IsFloat32NegZero(a) && b >= 0.0)
            {
                result.f32[i] = b;
            }
            else if (Js::NumberUtilities::IsFloat32NegZero(b) && a >= 0.0)
            {
                result.f32[i] = a;
            }
            else
            {
                result.f32[i] = a < b ? b : a;
            }
        }
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpScale(const SIMDValue& Value, float scaleValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(Value.f32);
        float32x4_t vs = vdupq_n_f32(scaleValue);
        vst1q_f32(result.f32, vmulq_f32(va, vs));
#else
        result.f32[SIMD_X] = Value.f32[SIMD_X] * scaleValue;
        result.f32[SIMD_Y] = Value.f32[SIMD_Y] * scaleValue;
        result.f32[SIMD_Z] = Value.f32[SIMD_Z] * scaleValue;
        result.f32[SIMD_W] = Value.f32[SIMD_W] * scaleValue;
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        uint32x4_t cmp = vcltq_f32(va, vb);
        // Convert to bool mask: 0xFFFFFFFF -> -1, 0x00000000 -> 0
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        int x = aValue.f32[SIMD_X] < bValue.f32[SIMD_X];
        int y = aValue.f32[SIMD_Y] < bValue.f32[SIMD_Y];
        int z = aValue.f32[SIMD_Z] < bValue.f32[SIMD_Z];
        int w = aValue.f32[SIMD_W] < bValue.f32[SIMD_W];

        result = SIMDInt32x4Operation::OpBool(x, y, z, w);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        uint32x4_t cmp = vcleq_f32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        int x = aValue.f32[SIMD_X] <= bValue.f32[SIMD_X];
        int y = aValue.f32[SIMD_Y] <= bValue.f32[SIMD_Y];
        int z = aValue.f32[SIMD_Z] <= bValue.f32[SIMD_Z];
        int w = aValue.f32[SIMD_W] <= bValue.f32[SIMD_W];

        result = SIMDInt32x4Operation::OpBool(x, y, z, w);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        uint32x4_t cmp = vceqq_f32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        int x = aValue.f32[SIMD_X] == bValue.f32[SIMD_X];
        int y = aValue.f32[SIMD_Y] == bValue.f32[SIMD_Y];
        int z = aValue.f32[SIMD_Z] == bValue.f32[SIMD_Z];
        int w = aValue.f32[SIMD_W] == bValue.f32[SIMD_W];

        result = SIMDInt32x4Operation::OpBool(x, y, z, w);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        uint32x4_t cmp = vceqq_f32(va, vb);
        // NOT the equal result to get not-equal
        vst1q_s32(result.i32, vreinterpretq_s32_u32(vmvnq_u32(cmp)));
#else
        int x = aValue.f32[SIMD_X] != bValue.f32[SIMD_X];
        int y = aValue.f32[SIMD_Y] != bValue.f32[SIMD_Y];
        int z = aValue.f32[SIMD_Z] != bValue.f32[SIMD_Z];
        int w = aValue.f32[SIMD_W] != bValue.f32[SIMD_W];

        result = SIMDInt32x4Operation::OpBool(x, y, z, w);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        uint32x4_t cmp = vcgtq_f32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        int x = aValue.f32[SIMD_X] > bValue.f32[SIMD_X];
        int y = aValue.f32[SIMD_Y] > bValue.f32[SIMD_Y];
        int z = aValue.f32[SIMD_Z] > bValue.f32[SIMD_Z];
        int w = aValue.f32[SIMD_W] > bValue.f32[SIMD_W];

        result = SIMDInt32x4Operation::OpBool(x, y, z, w);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t va = vld1q_f32(aValue.f32);
        float32x4_t vb = vld1q_f32(bValue.f32);
        uint32x4_t cmp = vcgeq_f32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        int x = aValue.f32[SIMD_X] >= bValue.f32[SIMD_X];
        int y = aValue.f32[SIMD_Y] >= bValue.f32[SIMD_Y];
        int z = aValue.f32[SIMD_Z] >= bValue.f32[SIMD_Z];
        int w = aValue.f32[SIMD_W] >= bValue.f32[SIMD_W];

        result = SIMDInt32x4Operation::OpBool(x, y, z, w);
#endif
        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpClamp(const SIMDValue& value, const SIMDValue& lower, const SIMDValue& upper)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float32x4_t vval = vld1q_f32(value.f32);
        float32x4_t vlo  = vld1q_f32(lower.f32);
        float32x4_t vhi  = vld1q_f32(upper.f32);
        // Clamp: max(lower, min(upper, value))
        float32x4_t clamped = vmaxq_f32(vlo, vminq_f32(vhi, vval));
        vst1q_f32(result.f32, clamped);
#else
        // lower clamp
        result.f32[SIMD_X] = value.f32[SIMD_X] < lower.f32[SIMD_X] ? lower.f32[SIMD_X] : value.f32[SIMD_X];
        result.f32[SIMD_Y] = value.f32[SIMD_Y] < lower.f32[SIMD_Y] ? lower.f32[SIMD_Y] : value.f32[SIMD_Y];
        result.f32[SIMD_Z] = value.f32[SIMD_Z] < lower.f32[SIMD_Z] ? lower.f32[SIMD_Z] : value.f32[SIMD_Z];
        result.f32[SIMD_W] = value.f32[SIMD_W] < lower.f32[SIMD_W] ? lower.f32[SIMD_W] : value.f32[SIMD_W];

        // upper clamp
        result.f32[SIMD_X] = result.f32[SIMD_X] > upper.f32[SIMD_X] ? upper.f32[SIMD_X] : result.f32[SIMD_X];
        result.f32[SIMD_Y] = result.f32[SIMD_Y] > upper.f32[SIMD_Y] ? upper.f32[SIMD_Y] : result.f32[SIMD_Y];
        result.f32[SIMD_Z] = result.f32[SIMD_Z] > upper.f32[SIMD_Z] ? upper.f32[SIMD_Z] : result.f32[SIMD_Z];
        result.f32[SIMD_W] = result.f32[SIMD_W] > upper.f32[SIMD_W] ? upper.f32[SIMD_W] : result.f32[SIMD_W];
#endif
        return result;
    }


    SIMDValue SIMDFloat32x4Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        // BSL: for each bit, if mask bit is 1 select from tV, else from fV
        uint32x4_t vm = vld1q_u32(mV.u32);
        uint32x4_t vt = vld1q_u32(tV.u32);
        uint32x4_t vf = vld1q_u32(fV.u32);
        vst1q_u32(result.u32, vbslq_u32(vm, vt, vf));
#else
        SIMDValue trueResult  = SIMDInt32x4Operation::OpAnd(mV, tV);
        SIMDValue notValue    = SIMDInt32x4Operation::OpNot(mV);
        SIMDValue falseResult = SIMDInt32x4Operation::OpAnd(notValue, fV);

        result = SIMDInt32x4Operation::OpOr(trueResult, falseResult);
#endif
        return result;
    }

}
#endif