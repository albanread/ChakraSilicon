//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDFloat64x2Operation::OpFloat64x2(double x, double y)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        double vals[2] = { x, y };
        float64x2_t v = vld1q_f64(vals);
        vst1q_f64(result.f64, v);
#else
        result.f64[SIMD_X] = x;
        result.f64[SIMD_Y] = y;
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpSplat(double x)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        vst1q_f64(result.f64, vdupq_n_f64(x));
#else
        result.f64[SIMD_X] = result.f64[SIMD_Y] = x;
#endif
        return result;
    }

    // Conversions
    SIMDValue SIMDFloat64x2Operation::OpFromFloat32x4(const SIMDValue& v)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        // Convert the low two float32 lanes to float64
        float32x2_t lo = vld1_f32(v.f32);
        vst1q_f64(result.f64, vcvt_f64_f32(lo));
#else
        result.f64[SIMD_X] = (double)(v.f32[SIMD_X]);
        result.f64[SIMD_Y] = (double)(v.f32[SIMD_Y]);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpFromInt32x4(const SIMDValue& v)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        // Convert low two int32 lanes to float32, then to float64
        int32x2_t lo = vld1_s32(v.i32);
        float32x2_t flo = vcvt_f32_s32(lo);
        vst1q_f64(result.f64, vcvt_f64_f32(flo));
#else
        result.f64[SIMD_X] = (double)(v.i32[SIMD_X]);
        result.f64[SIMD_Y] = (double)(v.i32[SIMD_Y]);
#endif
        return result;
    }

    // Unary Ops
    SIMDValue SIMDFloat64x2Operation::OpAbs(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(value.f64);
        vst1q_f64(result.f64, vabsq_f64(va));
#else
        result.f64[SIMD_X] = (value.f64[SIMD_X] < 0) ? -1 * value.f64[SIMD_X] : value.f64[SIMD_X];
        result.f64[SIMD_Y] = (value.f64[SIMD_Y] < 0) ? -1 * value.f64[SIMD_Y] : value.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(value.f64);
        vst1q_f64(result.f64, vnegq_f64(va));
#else
        result.f64[SIMD_X] = -1 * value.f64[SIMD_X];
        result.f64[SIMD_Y] = -1 * value.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpNot(const SIMDValue& value)
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

    SIMDValue SIMDFloat64x2Operation::OpReciprocal(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(value.f64);
        float64x2_t ones = vdupq_n_f64(1.0);
        vst1q_f64(result.f64, vdivq_f64(ones, va));
#else
        result.f64[SIMD_X] = 1.0/(value.f64[SIMD_X]);
        result.f64[SIMD_Y] = 1.0/(value.f64[SIMD_Y]);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpReciprocalSqrt(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(value.f64);
        float64x2_t ones = vdupq_n_f64(1.0);
        vst1q_f64(result.f64, vsqrtq_f64(vdivq_f64(ones, va)));
#else
        result.f64[SIMD_X] = sqrt(1.0 / (value.f64[SIMD_X]));
        result.f64[SIMD_Y] = sqrt(1.0 / (value.f64[SIMD_Y]));
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpSqrt(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(value.f64);
        vst1q_f64(result.f64, vsqrtq_f64(va));
#else
        result.f64[SIMD_X] = sqrt(value.f64[SIMD_X]);
        result.f64[SIMD_Y] = sqrt(value.f64[SIMD_Y]);
#endif
        return result;
    }

    // Binary Ops
    SIMDValue SIMDFloat64x2Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        vst1q_f64(result.f64, vaddq_f64(va, vb));
#else
        result.f64[SIMD_X] = aValue.f64[SIMD_X] + bValue.f64[SIMD_X];
        result.f64[SIMD_Y] = aValue.f64[SIMD_Y] + bValue.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        vst1q_f64(result.f64, vsubq_f64(va, vb));
#else
        result.f64[SIMD_X] = aValue.f64[SIMD_X] - bValue.f64[SIMD_X];
        result.f64[SIMD_Y] = aValue.f64[SIMD_Y] - bValue.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        vst1q_f64(result.f64, vmulq_f64(va, vb));
#else
        result.f64[SIMD_X] = aValue.f64[SIMD_X] * bValue.f64[SIMD_X];
        result.f64[SIMD_Y] = aValue.f64[SIMD_Y] * bValue.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpDiv(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        vst1q_f64(result.f64, vdivq_f64(va, vb));
#else
        result.f64[SIMD_X] = aValue.f64[SIMD_X] / bValue.f64[SIMD_X];
        result.f64[SIMD_Y] = aValue.f64[SIMD_Y] / bValue.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
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

    SIMDValue SIMDFloat64x2Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
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

    SIMDValue SIMDFloat64x2Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
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

    SIMDValue SIMDFloat64x2Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        vst1q_f64(result.f64, vminq_f64(va, vb));
#else
        result.f64[SIMD_X] = (aValue.f64[SIMD_X] < bValue.f64[SIMD_X]) ? aValue.f64[SIMD_X] : bValue.f64[SIMD_X];
        result.f64[SIMD_Y] = (aValue.f64[SIMD_Y] < bValue.f64[SIMD_Y]) ? aValue.f64[SIMD_Y] : bValue.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        vst1q_f64(result.f64, vmaxq_f64(va, vb));
#else
        result.f64[SIMD_X] = (aValue.f64[SIMD_X] > bValue.f64[SIMD_X]) ? aValue.f64[SIMD_X] : bValue.f64[SIMD_X];
        result.f64[SIMD_Y] = (aValue.f64[SIMD_Y] > bValue.f64[SIMD_Y]) ? aValue.f64[SIMD_Y] : bValue.f64[SIMD_Y];
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpScale(const SIMDValue& Value, double scaleValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(Value.f64);
        float64x2_t vs = vdupq_n_f64(scaleValue);
        vst1q_f64(result.f64, vmulq_f64(va, vs));
#else
        result.f64[SIMD_X] = Value.f64[SIMD_X] * scaleValue;
        result.f64[SIMD_Y] = Value.f64[SIMD_Y] * scaleValue;
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        uint64x2_t cmp = vcltq_f64(va, vb);
        // Convert to i32 bool mask: each 64-bit result maps to two identical i32 lanes
        uint32x4_t cmp32 = vreinterpretq_u32_u64(cmp);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp32));
#else
        int x = aValue.f64[SIMD_X] < bValue.f64[SIMD_X];
        int y = aValue.f64[SIMD_Y] < bValue.f64[SIMD_Y];

        result = SIMDInt32x4Operation::OpBool(x, x, y, y);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        uint64x2_t cmp = vcleq_f64(va, vb);
        uint32x4_t cmp32 = vreinterpretq_u32_u64(cmp);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp32));
#else
        int x = aValue.f64[SIMD_X] <= bValue.f64[SIMD_X];
        int y = aValue.f64[SIMD_Y] <= bValue.f64[SIMD_Y];

        result = SIMDInt32x4Operation::OpBool(x, x, y, y);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        uint64x2_t cmp = vceqq_f64(va, vb);
        uint32x4_t cmp32 = vreinterpretq_u32_u64(cmp);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp32));
#else
        int x = aValue.f64[SIMD_X] == bValue.f64[SIMD_X];
        int y = aValue.f64[SIMD_Y] == bValue.f64[SIMD_Y];

        result = SIMDInt32x4Operation::OpBool(x, x, y, y);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        uint64x2_t cmp = vceqq_f64(va, vb);
        // NOT to get not-equal
        uint32x4_t cmp32 = vreinterpretq_u32_u64(cmp);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(vmvnq_u32(cmp32)));
#else
        int x = aValue.f64[SIMD_X] != bValue.f64[SIMD_X];
        int y = aValue.f64[SIMD_Y] != bValue.f64[SIMD_Y];

        result = SIMDInt32x4Operation::OpBool(x, x, y, y);
#endif
        return result;
    }


    SIMDValue SIMDFloat64x2Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        uint64x2_t cmp = vcgtq_f64(va, vb);
        uint32x4_t cmp32 = vreinterpretq_u32_u64(cmp);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp32));
#else
        int x = aValue.f64[SIMD_X] > bValue.f64[SIMD_X];
        int y = aValue.f64[SIMD_Y] > bValue.f64[SIMD_Y];

        result = SIMDInt32x4Operation::OpBool(x, x, y, y);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        float64x2_t va = vld1q_f64(aValue.f64);
        float64x2_t vb = vld1q_f64(bValue.f64);
        uint64x2_t cmp = vcgeq_f64(va, vb);
        uint32x4_t cmp32 = vreinterpretq_u32_u64(cmp);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp32));
#else
        int x = aValue.f64[SIMD_X] >= bValue.f64[SIMD_X];
        int y = aValue.f64[SIMD_Y] >= bValue.f64[SIMD_Y];

        result = SIMDInt32x4Operation::OpBool(x, x, y, y);
#endif
        return result;
    }

    SIMDValue SIMDFloat64x2Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
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