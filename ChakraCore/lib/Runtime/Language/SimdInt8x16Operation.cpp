//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDInt8x16Operation::OpInt8x16(int8 values[])
    {
        SIMDValue result = {0, 0, 0, 0};

#if CHAKRA_NEON_AVAILABLE
        int8x16_t v = vld1q_s8(reinterpret_cast<const int8_t*>(values));
        vst1q_s8(result.i8, v);
#else
        for (uint8 i = 0; i < 16; i++)
        {
            result.i8[i] = values[i];
        }
#endif
        
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSplat(int8 x)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        vst1q_s8(result.i8, vdupq_n_s8(x));
#else
        result.i8[0] = result.i8[1] = result.i8[2] = result.i8[3] = result.i8[4] = result.i8[5] = result.i8[6]= result.i8[7] = result.i8[8] = result.i8[9]= result.i8[10] = result.i8[11] = result.i8[12]= result.i8[13] = result.i8[14] = result.i8[15] = x;
#endif

        return result;
    }

    //// Unary Ops
    SIMDValue SIMDInt8x16Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(value.i8);
        vst1q_s8(result.i8, vnegq_s8(va));
#else
        result.i8[0]  = -1 * value.i8[0];
        result.i8[1]  = -1 * value.i8[1];
        result.i8[2]  = -1 * value.i8[2];
        result.i8[3]  = -1 * value.i8[3];
        result.i8[4]  = -1 * value.i8[4];
        result.i8[5]  = -1 * value.i8[5];
        result.i8[6]  = -1 * value.i8[6];
        result.i8[7]  = -1 * value.i8[7];
        result.i8[8]  = -1 * value.i8[8];
        result.i8[9]  = -1 * value.i8[9];
        result.i8[10] = -1 * value.i8[10];
        result.i8[11] = -1 * value.i8[11];
        result.i8[12] = -1 * value.i8[12];
        result.i8[13] = -1 * value.i8[13];
        result.i8[14] = -1 * value.i8[14];
        result.i8[15] = -1 * value.i8[15];
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vaddq_s8(va, vb));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = aValue.i8[idx] + bValue.i8[idx];
        }
#endif
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vsubq_s8(va, vb));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = aValue.i8[idx] - bValue.i8[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vmulq_s8(va, vb));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = aValue.i8[idx] * bValue.i8[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vqaddq_s8(va, vb));
#else
        int mask = 0x80;
        for (uint idx = 0; idx < 16; ++idx)
        {
            int8 val1 = aValue.i8[idx];
            int8 val2 = bValue.i8[idx];
            int8 sum = val1 + val2;

            result.i8[idx] = sum;
            if (val1 > 0 && val2 > 0 && sum < 0)
            {
                result.i8[idx] = 0x7F;
            }
            else if (val1 < 0 && val2 < 0 && sum > 0)
            {
                result.i8[idx] = static_cast<int8>(mask);
            }
        }
#endif
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vqsubq_s8(va, vb));
#else
        int mask = 0x80;
        for (uint idx = 0; idx < 16; ++idx)
        {
            int8 val1 = aValue.i8[idx];
            int8 val2 = bValue.i8[idx];
            int16 diff = val1 + val2;

            result.i8[idx] = static_cast<int8>(diff);
            if (diff > 0x7F)
            {
                result.i8[idx] = 0x7F;
            }
            else if (diff < 0x80)
            {
                result.i8[idx] = static_cast<int8>(mask);
            }
        }
#endif
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vminq_s8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] < bValue.i8[idx]) ? aValue.i8[idx] : bValue.i8[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vmaxq_s8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] > bValue.i8[idx]) ? aValue.i8[idx] : bValue.i8[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vreinterpretq_s8_u8(vcltq_s8(va, vb)));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] < bValue.i8[idx]) ? 0xff : 0x0;
        }
#endif
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vreinterpretq_s8_u8(vcleq_s8(va, vb)));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] <= bValue.i8[idx]) ? 0xff : 0x0;
        }
#endif
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vreinterpretq_s8_u8(vceqq_s8(va, vb)));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] == bValue.i8[idx]) ? 0xff : 0x0;
        }
#endif

        return result;
    }


    SIMDValue SIMDInt8x16Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        // NotEqual = bitwise NOT of Equal
        uint8x16_t eq = vceqq_s8(va, vb);
        vst1q_s8(result.i8, vreinterpretq_s8_u8(vmvnq_u8(eq)));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] != bValue.i8[idx]) ? 0xff : 0x0;
        }
#endif

        return result;
    }


    SIMDValue SIMDInt8x16Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vreinterpretq_s8_u8(vcgtq_s8(va, vb)));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] > bValue.i8[idx]) ? 0xff : 0x0;
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(aValue.i8);
        int8x16_t vb = vld1q_s8(bValue.i8);
        vst1q_s8(result.i8, vreinterpretq_s8_u8(vcgeq_s8(va, vb)));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] >= bValue.i8[idx]) ? 0xff : 0x0;
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpShiftLeftByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(1);

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(value.i8);
        int8x16_t vcount = vdupq_n_s8(static_cast<int8_t>(count));
        vst1q_s8(result.i8, vshlq_s8(va, vcount));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = value.i8[idx] << count;
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(1);

#if CHAKRA_NEON_AVAILABLE
        int8x16_t va = vld1q_s8(value.i8);
        // Negative shift count = right shift for vshlq
        int8x16_t vcount = vdupq_n_s8(static_cast<int8_t>(-count));
        vst1q_s8(result.i8, vshlq_s8(va, vcount));
#else
        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = value.i8[idx] >> count;
        }
#endif

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        // Use bitwise select: for each bit, if mask bit is 1 pick tV, else fV
        // The original scalar code checks (mV.i8[idx] == 1), but the SIMD semantics
        // use all-ones (-1 / 0xFF) masks, so we use vbslq which does bitwise selection.
        uint8x16_t mask = vld1q_u8(reinterpret_cast<const uint8_t*>(mV.i8));
        uint8x16_t trueVals = vld1q_u8(reinterpret_cast<const uint8_t*>(tV.i8));
        uint8x16_t falseVals = vld1q_u8(reinterpret_cast<const uint8_t*>(fV.i8));
        vst1q_u8(reinterpret_cast<uint8_t*>(result.i8), vbslq_u8(mask, trueVals, falseVals));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (mV.i8[idx] == 1) ? tV.i8[idx] : fV.i8[idx];
        }
#endif
        return result;
    }

}
#endif