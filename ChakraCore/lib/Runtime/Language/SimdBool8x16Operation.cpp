//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDBool8x16Operation::OpBool8x16(bool b[])
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int8_t vals[16];
        for (uint i = 0; i < 16; i++)
        {
            vals[i] = b[i] ? -1 : 0;
        }
        vst1q_s8(result.i8, vld1q_s8(vals));
#else
        for (uint i = 0; i < 16; i++)
        {
            result.i8[i] = b[i] ? -1 : 0;
        }
#endif

        return result;
    }

    SIMDValue SIMDBool8x16Operation::OpBool8x16(const SIMDValue& v)
    {
        // overload function with input parameter as SIMDValue for completeness
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        vst1q_s8(result.i8, vld1q_s8(v.i8));
#else
        result = v;
#endif
        return result;
    }
}
#endif