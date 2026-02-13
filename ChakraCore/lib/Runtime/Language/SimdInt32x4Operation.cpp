//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDInt32x4Operation::OpInt32x4(int x, int y, int z, int w)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32_t vals[4] = { x, y, z, w };
        int32x4_t v = vld1q_s32(vals);
        vst1q_s32(result.i32, v);
#else
        result.i32[SIMD_X] = x;
        result.i32[SIMD_Y] = y;
        result.i32[SIMD_Z] = z;
        result.i32[SIMD_W] = w;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpSplat(int x)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        vst1q_s32(result.i32, vdupq_n_s32(x));
#else
        result.i32[SIMD_X] = result.i32[SIMD_Y] = result.i32[SIMD_Z] = result.i32[SIMD_W] = x;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpBool(int x, int y, int z, int w)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32_t vals[4] = { x ? -1 : 0, y ? -1 : 0, z ? -1 : 0, w ? -1 : 0 };
        vst1q_s32(result.i32, vld1q_s32(vals));
#else
        int nX = x ? -1 : 0x0;
        int nY = y ? -1 : 0x0;
        int nZ = z ? -1 : 0x0;
        int nW = w ? -1 : 0x0;

        result.i32[SIMD_X] = nX;
        result.i32[SIMD_Y] = nY;
        result.i32[SIMD_Z] = nZ;
        result.i32[SIMD_W] = nW;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpBool(const SIMDValue& v)
    {
        SIMDValue result;

        // incoming 4 signed integers has to be 0 or -1
        Assert(v.i32[SIMD_X] == 0 || v.i32[SIMD_X] == -1);
        Assert(v.i32[SIMD_Y] == 0 || v.i32[SIMD_Y] == -1);
        Assert(v.i32[SIMD_Z] == 0 || v.i32[SIMD_Z] == -1);
        Assert(v.i32[SIMD_W] == 0 || v.i32[SIMD_W] == -1);
        result = v;
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpFromFloat32x4(const SIMDValue& v, bool &throws)
    {
        SIMDValue result = { 0 };
        const float MIN_INT = -2147483648.0f;
        const float MAX_INT_PLUS_1 = 2147483648.0f;  // exact float

        for (uint i = 0; i < 4; i++)
        {
            if (v.f32[i] >= MIN_INT && v.f32[i] < MAX_INT_PLUS_1)
            {
                result.u32[i] = (int)(v.f32[i]);
            }
            else
            {
                // out of range. Caller should throw.
                throws = true;
                return result;
            }
        }
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpFromFloat64x2(const SIMDValue& v)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        // Convert two doubles to two int32s, zero the upper two lanes
        float64x2_t vd = vld1q_f64(v.f64);
        int32x2_t lo = vcvt_s32_f32(vcvt_f32_f64(vd));
        int32x4_t full = vcombine_s32(lo, vdup_n_s32(0));
        vst1q_s32(result.i32, full);
#else
        result.i32[SIMD_X] = (int)(v.f64[SIMD_X]);
        result.i32[SIMD_Y] = (int)(v.f64[SIMD_Y]);
        result.i32[SIMD_Z] = result.i32[SIMD_W] = 0;
#endif
        return result;
    }


    // Unary Ops
    SIMDValue SIMDInt32x4Operation::OpAbs(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(value.i32);
        vst1q_s32(result.i32, vabsq_s32(va));
#else
        result.i32[SIMD_X] = (value.i32[SIMD_X] < 0) ? -1 * value.i32[SIMD_X] : value.i32[SIMD_X];
        result.i32[SIMD_Y] = (value.i32[SIMD_Y] < 0) ? -1 * value.i32[SIMD_Y] : value.i32[SIMD_Y];
        result.i32[SIMD_Z] = (value.i32[SIMD_Z] < 0) ? -1 * value.i32[SIMD_Z] : value.i32[SIMD_Z];
        result.i32[SIMD_W] = (value.i32[SIMD_W] < 0) ? -1 * value.i32[SIMD_W] : value.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(value.i32);
        vst1q_s32(result.i32, vnegq_s32(va));
#else
        result.i32[SIMD_X] = -1 * value.i32[SIMD_X];
        result.i32[SIMD_Y] = -1 * value.i32[SIMD_Y];
        result.i32[SIMD_Z] = -1 * value.i32[SIMD_Z];
        result.i32[SIMD_W] = -1 * value.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpNot(const SIMDValue& value)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(value.i32);
        vst1q_s32(result.i32, vmvnq_s32(va));
#else
        result.i32[SIMD_X] = ~(value.i32[SIMD_X]);
        result.i32[SIMD_Y] = ~(value.i32[SIMD_Y]);
        result.i32[SIMD_Z] = ~(value.i32[SIMD_Z]);
        result.i32[SIMD_W] = ~(value.i32[SIMD_W]);
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vaddq_s32(va, vb));
#else
        result.i32[SIMD_X] = aValue.i32[SIMD_X] + bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = aValue.i32[SIMD_Y] + bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = aValue.i32[SIMD_Z] + bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = aValue.i32[SIMD_W] + bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vsubq_s32(va, vb));
#else
        result.i32[SIMD_X] = aValue.i32[SIMD_X] - bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = aValue.i32[SIMD_Y] - bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = aValue.i32[SIMD_Z] - bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = aValue.i32[SIMD_W] - bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vmulq_s32(va, vb));
#else
        result.i32[SIMD_X] = aValue.i32[SIMD_X] * bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = aValue.i32[SIMD_Y] * bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = aValue.i32[SIMD_Z] * bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = aValue.i32[SIMD_W] * bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vandq_s32(va, vb));
#else
        result.i32[SIMD_X] = aValue.i32[SIMD_X] & bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = aValue.i32[SIMD_Y] & bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = aValue.i32[SIMD_Z] & bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = aValue.i32[SIMD_W] & bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vorrq_s32(va, vb));
#else
        result.i32[SIMD_X] = aValue.i32[SIMD_X] | bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = aValue.i32[SIMD_Y] | bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = aValue.i32[SIMD_Z] | bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = aValue.i32[SIMD_W] | bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, veorq_s32(va, vb));
#else
        result.i32[SIMD_X] = aValue.i32[SIMD_X] ^ bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = aValue.i32[SIMD_Y] ^ bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = aValue.i32[SIMD_Z] ^ bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = aValue.i32[SIMD_W] ^ bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vminq_s32(va, vb));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] < bValue.i32[SIMD_X]) ? aValue.i32[SIMD_X] : bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] < bValue.i32[SIMD_Y]) ? aValue.i32[SIMD_Y] : bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] < bValue.i32[SIMD_Z]) ? aValue.i32[SIMD_Z] : bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] < bValue.i32[SIMD_W]) ? aValue.i32[SIMD_W] : bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        vst1q_s32(result.i32, vmaxq_s32(va, vb));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] > bValue.i32[SIMD_X]) ? aValue.i32[SIMD_X] : bValue.i32[SIMD_X];
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] > bValue.i32[SIMD_Y]) ? aValue.i32[SIMD_Y] : bValue.i32[SIMD_Y];
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] > bValue.i32[SIMD_Z]) ? aValue.i32[SIMD_Z] : bValue.i32[SIMD_Z];
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] > bValue.i32[SIMD_W]) ? aValue.i32[SIMD_W] : bValue.i32[SIMD_W];
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        uint32x4_t cmp = vcltq_s32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] < bValue.i32[SIMD_X]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] < bValue.i32[SIMD_Y]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] < bValue.i32[SIMD_Z]) ? 0xffffffff : 0x0;
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] < bValue.i32[SIMD_W]) ? 0xffffffff : 0x0;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        uint32x4_t cmp = vcleq_s32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] <= bValue.i32[SIMD_X]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] <= bValue.i32[SIMD_Y]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] <= bValue.i32[SIMD_Z]) ? 0xffffffff : 0x0;
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] <= bValue.i32[SIMD_W]) ? 0xffffffff : 0x0;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        uint32x4_t cmp = vceqq_s32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] == bValue.i32[SIMD_X]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] == bValue.i32[SIMD_Y]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] == bValue.i32[SIMD_Z]) ? 0xffffffff : 0x0;
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] == bValue.i32[SIMD_W]) ? 0xffffffff : 0x0;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        uint32x4_t cmp = vceqq_s32(va, vb);
        // NOT the equal result to get not-equal
        vst1q_s32(result.i32, vreinterpretq_s32_u32(vmvnq_u32(cmp)));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] != bValue.i32[SIMD_X]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] != bValue.i32[SIMD_Y]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] != bValue.i32[SIMD_Z]) ? 0xffffffff : 0x0;
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] != bValue.i32[SIMD_W]) ? 0xffffffff : 0x0;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        uint32x4_t cmp = vcgtq_s32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] > bValue.i32[SIMD_X]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] > bValue.i32[SIMD_Y]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] > bValue.i32[SIMD_Z]) ? 0xffffffff : 0x0;
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] > bValue.i32[SIMD_W]) ? 0xffffffff : 0x0;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(aValue.i32);
        int32x4_t vb = vld1q_s32(bValue.i32);
        uint32x4_t cmp = vcgeq_s32(va, vb);
        vst1q_s32(result.i32, vreinterpretq_s32_u32(cmp));
#else
        result.i32[SIMD_X] = (aValue.i32[SIMD_X] >= bValue.i32[SIMD_X]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Y] = (aValue.i32[SIMD_Y] >= bValue.i32[SIMD_Y]) ? 0xffffffff : 0x0;
        result.i32[SIMD_Z] = (aValue.i32[SIMD_Z] >= bValue.i32[SIMD_Z]) ? 0xffffffff : 0x0;
        result.i32[SIMD_W] = (aValue.i32[SIMD_W] >= bValue.i32[SIMD_W]) ? 0xffffffff : 0x0;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpShiftLeftByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(4);

#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(value.i32);
        int32x4_t vcount = vdupq_n_s32(count);
        vst1q_s32(result.i32, vshlq_s32(va, vcount));
#else
        result.i32[SIMD_X] = value.i32[SIMD_X] << count;
        result.i32[SIMD_Y] = value.i32[SIMD_Y] << count;
        result.i32[SIMD_Z] = value.i32[SIMD_Z] << count;
        result.i32[SIMD_W] = value.i32[SIMD_W] << count;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(4);

#if CHAKRA_NEON_AVAILABLE
        int32x4_t va = vld1q_s32(value.i32);
        // Negative shift amount = arithmetic right shift in vshlq
        int32x4_t vcount = vdupq_n_s32(-count);
        vst1q_s32(result.i32, vshlq_s32(va, vcount));
#else
        result.i32[SIMD_X] = value.i32[SIMD_X] >> count;
        result.i32[SIMD_Y] = value.i32[SIMD_Y] >> count;
        result.i32[SIMD_Z] = value.i32[SIMD_Z] >> count;
        result.i32[SIMD_W] = value.i32[SIMD_W] >> count;
#endif
        return result;
    }

    SIMDValue SIMDInt32x4Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
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