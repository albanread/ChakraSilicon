//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDUint16x8Operation::OpUint16x8(uint16 values[])
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t v = vld1q_u16(reinterpret_cast<const uint16_t*>(values));
        vst1q_u16(result.u16, v);
#else
        for (uint i = 0; i < 8; i++)
        {
            result.u16[i] = values[i];
        }
#endif
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vminq_u16(va, vb));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] < bValue.u16[idx]) ? aValue.u16[idx] : bValue.u16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vmaxq_u16(va, vb));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] > bValue.u16[idx]) ? aValue.u16[idx] : bValue.u16[idx];
        }
#endif

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vcltq_u16(va, vb));
#else
        for(uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] < bValue.u16[idx]) ? 0xffff : 0x0;
        }
#endif
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vcleq_u16(va, vb));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] <= bValue.u16[idx]) ? 0xffff : 0x0;
        }
#endif
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vcgeq_u16(va, vb));
#else
        result = SIMDUint16x8Operation::OpLessThan(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
#endif

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vcgtq_u16(va, vb));
#else
        result = SIMDUint16x8Operation::OpLessThanOrEqual(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
#endif

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(2);

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(value.u16);
        int16x8_t vcount = vdupq_n_s16(static_cast<int16_t>(-count)); // negative for right shift
        vst1q_u16(result.u16, vshlq_u16(va, vcount));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (value.u16[idx] >> count);
        }
#endif
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vqaddq_u16(va, vb));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            uint32 a = (uint32)aValue.u16[idx];
            uint32 b = (uint32)bValue.u16[idx];

            result.u16[idx] = ((a + b) > MAXUINT16) ? MAXUINT16 : (uint16)(a + b);
        }
#endif
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        uint16x8_t va = vld1q_u16(aValue.u16);
        uint16x8_t vb = vld1q_u16(bValue.u16);
        vst1q_u16(result.u16, vqsubq_u16(va, vb));
#else
        for (uint idx = 0; idx < 8; ++idx)
        {
            int a = (int)aValue.u16[idx];
            int b = (int)bValue.u16[idx];

            result.u16[idx] = ((a - b) < 0) ? 0 : (uint16)(a - b);
        }
#endif
        return result;
    }
}

#endif