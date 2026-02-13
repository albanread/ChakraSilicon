//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDUint8x16Operation::OpUint8x16(uint8 values[])
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t*>(values));
        vst1q_u8(result.u8, v);
#else
        for (uint i = 0; i < 16; i++)
        {
            result.u8[i] = values[i];
        }
#endif
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vminq_u8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] < bValue.u8[idx]) ? aValue.u8[idx] : bValue.u8[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vmaxq_u8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] > bValue.u8[idx]) ? aValue.u8[idx] : bValue.u8[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vcltq_u8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] < bValue.u8[idx]) ? 0xff : 0x0;
        }
#endif
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vcleq_u8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] <= bValue.u8[idx]) ? 0xff : 0x0;
        }
#endif
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vcgeq_u8(va, vb));
#else
        result = SIMDUint8x16Operation::OpLessThan(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
#endif

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vcgtq_u8(va, vb));
#else
        result = SIMDUint8x16Operation::OpLessThanOrEqual(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
#endif

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(1);

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(value.u8);
        int8x16_t vcount = vdupq_n_s8(static_cast<int8_t>(-count)); // negative for right shift
        vst1q_u8(result.u8, vshlq_u8(va, vcount));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (value.u8[idx] >> count);
        }
#endif
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vqaddq_u8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            uint16 a = (uint16)aValue.u8[idx];
            uint16 b = (uint16)bValue.u8[idx];

            result.u8[idx] = ((a + b) > MAXUINT8) ? MAXUINT8 : (uint8)(a + b);
        }
#endif
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint8x16_t va = vld1q_u8(aValue.u8);
        uint8x16_t vb = vld1q_u8(bValue.u8);
        vst1q_u8(result.u8, vqsubq_u8(va, vb));
#else
        for (uint idx = 0; idx < 16; ++idx)
        {
            int a = (int)aValue.u8[idx];
            int b = (int)bValue.u8[idx];

            result.u8[idx] = ((a - b) < 0) ? 0 : (uint8)(a - b);
        }
#endif
        return result;
    }
}

#endif