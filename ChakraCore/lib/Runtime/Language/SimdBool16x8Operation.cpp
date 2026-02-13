//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDBool16x8Operation::OpBool16x8(bool b[])
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int16_t vals[8];
        for (uint i = 0; i < 8; i++)
        {
            vals[i] = b[i] ? -1 : 0;
        }
        vst1q_s16(result.i16, vld1q_s16(vals));
#else
        for (uint i = 0; i < 8; i++)
        {
            result.i16[i] = b[i] ? -1 : 0;
        }
#endif

        return result;
    }

    SIMDValue SIMDBool16x8Operation::OpBool16x8(const SIMDValue& v)
    {
        // overload function with input parameter as SIMDValue for completeness
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        vst1q_s16(result.i16, vld1q_s16(v.i16));
#else
        result = v;
#endif
        return result;
    }
}
#endif