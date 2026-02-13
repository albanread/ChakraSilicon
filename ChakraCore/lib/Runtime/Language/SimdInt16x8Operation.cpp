//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"
#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDInt16x8Operation::OpInt16x8(int16 values[])
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int16x8_t v = vld1q_s16(reinterpret_cast<const int16_t*>(values));
        vst1q_s16(result.i16, v);
#else
        for (uint i = 0; i < 8; i ++)
        { 
            result.i16[i] = values[i];
        }
#endif
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpSplat(int16 x)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        vst1q_s16(result.i16, vdupq_n_s16(x));
#else
        result.i16[0] = result.i16[1] = result.i16[2] = result.i16[3] = x;
        result.i16[4] = result.i16[5] = result.i16[6] = result.i16[7] = x;
#endif

        return result;
    }

    // Unary Ops
    SIMDValue SIMDInt16x8Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(value.i16);
        vst1q_s16(result.i16, vnegq_s16(va));
#else
        result.i16[0] = -1 * value.i16[0];
        result.i16[1] = -1 * value.i16[1];
        result.i16[2] = -1 * value.i16[2];
        result.i16[3] = -1 * value.i16[3];
        result.i16[4] = -1 * value.i16[4];
        result.i16[5] = -1 * value.i16[5];
        result.i16[6] = -1 * value.i16[6];
        result.i16[7] = -1 * value.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpNot(const SIMDValue& value)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(value.i16);
        vst1q_s16(result.i16, vmvnq_s16(va));
#else
        result.i16[0] = ~(value.i16[0]);
        result.i16[1] = ~(value.i16[1]);
        result.i16[2] = ~(value.i16[2]);
        result.i16[3] = ~(value.i16[3]);
        result.i16[4] = ~(value.i16[4]);
        result.i16[5] = ~(value.i16[5]);
        result.i16[6] = ~(value.i16[6]);
        result.i16[7] = ~(value.i16[7]);
#endif

        return result;
    }

    // Binary Ops
    SIMDValue SIMDInt16x8Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vaddq_s16(va, vb));
#else
        result.i16[0] = aValue.i16[0] + bValue.i16[0];
        result.i16[1] = aValue.i16[1] + bValue.i16[1];
        result.i16[2] = aValue.i16[2] + bValue.i16[2];
        result.i16[3] = aValue.i16[3] + bValue.i16[3];
        result.i16[4] = aValue.i16[4] + bValue.i16[4];
        result.i16[5] = aValue.i16[5] + bValue.i16[5];
        result.i16[6] = aValue.i16[6] + bValue.i16[6];
        result.i16[7] = aValue.i16[7] + bValue.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vsubq_s16(va, vb));
#else
        result.i16[0] = aValue.i16[0] - bValue.i16[0];
        result.i16[1] = aValue.i16[1] - bValue.i16[1];
        result.i16[2] = aValue.i16[2] - bValue.i16[2];
        result.i16[3] = aValue.i16[3] - bValue.i16[3];
        result.i16[4] = aValue.i16[4] - bValue.i16[4];
        result.i16[5] = aValue.i16[5] - bValue.i16[5];
        result.i16[6] = aValue.i16[6] - bValue.i16[6];
        result.i16[7] = aValue.i16[7] - bValue.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vmulq_s16(va, vb));
#else
        result.i16[0] = aValue.i16[0] * bValue.i16[0];
        result.i16[1] = aValue.i16[1] * bValue.i16[1];
        result.i16[2] = aValue.i16[2] * bValue.i16[2];
        result.i16[3] = aValue.i16[3] * bValue.i16[3];
        result.i16[4] = aValue.i16[4] * bValue.i16[4];
        result.i16[5] = aValue.i16[5] * bValue.i16[5];
        result.i16[6] = aValue.i16[6] * bValue.i16[6];
        result.i16[7] = aValue.i16[7] * bValue.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vandq_s16(va, vb));
#else
        result.i16[0] = aValue.i16[0] & bValue.i16[0];
        result.i16[1] = aValue.i16[1] & bValue.i16[1];
        result.i16[2] = aValue.i16[2] & bValue.i16[2];
        result.i16[3] = aValue.i16[3] & bValue.i16[3];
        result.i16[4] = aValue.i16[4] & bValue.i16[4];
        result.i16[5] = aValue.i16[5] & bValue.i16[5];
        result.i16[6] = aValue.i16[6] & bValue.i16[6];
        result.i16[7] = aValue.i16[7] & bValue.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vorrq_s16(va, vb));
#else
        result.i16[0] = aValue.i16[0] | bValue.i16[0];
        result.i16[1] = aValue.i16[1] | bValue.i16[1];
        result.i16[2] = aValue.i16[2] | bValue.i16[2];
        result.i16[3] = aValue.i16[3] | bValue.i16[3];
        result.i16[4] = aValue.i16[4] | bValue.i16[4];
        result.i16[5] = aValue.i16[5] | bValue.i16[5];
        result.i16[6] = aValue.i16[6] | bValue.i16[6];
        result.i16[7] = aValue.i16[7] | bValue.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, veorq_s16(va, vb));
#else
        result.i16[0] = aValue.i16[0] ^ bValue.i16[0];
        result.i16[1] = aValue.i16[1] ^ bValue.i16[1];
        result.i16[2] = aValue.i16[2] ^ bValue.i16[2];
        result.i16[3] = aValue.i16[3] ^ bValue.i16[3];
        result.i16[4] = aValue.i16[4] ^ bValue.i16[4];
        result.i16[5] = aValue.i16[5] ^ bValue.i16[5];
        result.i16[6] = aValue.i16[6] ^ bValue.i16[6];
        result.i16[7] = aValue.i16[7] ^ bValue.i16[7];
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vqaddq_s16(va, vb));
#else
        int mask = 0x8000;
        for (uint idx = 0; idx < 8; ++idx)
        {
            int16 val1 = aValue.i16[idx];
            int16 val2 = bValue.i16[idx];
            int16 sum = val1 + val2;

            result.i16[idx] = sum;
            if (val1 > 0 && val2 > 0 && sum < 0)
            {
                result.i16[idx] = 0x7fff;
            }
            else if (val1 < 0 && val2 < 0 && sum > 0)
            {
                result.i16[idx] = static_cast<int16>(mask);
            }
        }
#endif
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vqsubq_s16(va, vb));
#else
        int mask = 0x8000;
        for (uint idx = 0; idx < 8; ++idx)
        {
            int16 val1 = aValue.i16[idx];
            int16 val2 = bValue.i16[idx];
            int16 diff = val1 + val2;

            result.i16[idx] = static_cast<int16>(diff);
            if (diff > 0x7fff)
                result.i16[idx] = 0x7fff;
            else if (diff < 0x8000)
                result.i16[idx] = static_cast<int16>(mask);
        }
#endif
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vminq_s16(va, vb));
#else
        for (int idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] < bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vmaxq_s16(va, vb));
#else
        for (int idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] > bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    // compare ops
    SIMDValue SIMDInt16x8Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vreinterpretq_s16_u16(vcltq_s16(va, vb)));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] < bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vreinterpretq_s16_u16(vcleq_s16(va, vb)));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] <= bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vreinterpretq_s16_u16(vceqq_s16(va, vb)));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] == bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        // NotEqual = bitwise NOT of Equal
        uint16x8_t eq = vceqq_s16(va, vb);
        vst1q_s16(result.i16, vreinterpretq_s16_u16(vmvnq_u16(eq)));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] != bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vreinterpretq_s16_u16(vcgtq_s16(va, vb)));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] > bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(aValue.i16);
        int16x8_t vb = vld1q_s16(bValue.i16);
        vst1q_s16(result.i16, vreinterpretq_s16_u16(vcgeq_s16(va, vb)));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] >= bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
#endif

        return result;
    }

    // ShiftOps
    SIMDValue SIMDInt16x8Operation::OpShiftLeftByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(2);

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(value.i16);
        int16x8_t vcount = vdupq_n_s16(static_cast<int16_t>(count));
        vst1q_s16(result.i16, vshlq_s16(va, vcount));
#else
        result.i16[0] = value.i16[0] << count;
        result.i16[1] = value.i16[1] << count;
        result.i16[2] = value.i16[2] << count;
        result.i16[3] = value.i16[3] << count;
        result.i16[4] = value.i16[4] << count;
        result.i16[5] = value.i16[5] << count;
        result.i16[6] = value.i16[6] << count;
        result.i16[7] = value.i16[7] << count;
#endif

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(2);

#if CHAKRA_NEON_AVAILABLE
        int16x8_t va = vld1q_s16(value.i16);
        // Negative shift count = right shift for vshlq
        int16x8_t vcount = vdupq_n_s16(static_cast<int16_t>(-count));
        vst1q_s16(result.i16, vshlq_s16(va, vcount));
#else
        result.i16[0] = value.i16[0] >> count;
        result.i16[1] = value.i16[1] >> count;
        result.i16[2] = value.i16[2] >> count;
        result.i16[3] = value.i16[3] >> count;
        result.i16[4] = value.i16[4] >> count;
        result.i16[5] = value.i16[5] >> count;
        result.i16[6] = value.i16[6] >> count;
        result.i16[7] = value.i16[7] >> count;
#endif

        return result;
    }

}

#endif