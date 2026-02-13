//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDBool32x4Operation::OpBool32x4(bool x, bool y, bool z, bool w)
    {
        SIMDValue result;

#if CHAKRA_NEON_AVAILABLE
        int32_t vals[4] = {
            x ? -1 : 0,
            y ? -1 : 0,
            z ? -1 : 0,
            w ? -1 : 0
        };
        vst1q_s32(result.i32, vld1q_s32(vals));
#else
        result.i32[SIMD_X] = x ? -1 : 0;
        result.i32[SIMD_Y] = y ? -1 : 0;
        result.i32[SIMD_Z] = z ? -1 : 0;
        result.i32[SIMD_W] = w ? -1 : 0;
#endif

        return result;
    }

    SIMDValue SIMDBool32x4Operation::OpBool32x4(const SIMDValue& v)
    {
        // overload function with input parameter as SIMDValue for completeness
        SIMDValue result;
        result = v;
        return result;
    }

    // Unary Ops
    template <typename T>
    bool SIMDBool32x4Operation::OpAnyTrue(const SIMDValue& val)
    {
        SIMDValue simd = SIMDUtils::CanonicalizeToBools<T>(val); //copy-by-value since we need to modify the copy

#if CHAKRA_NEON_AVAILABLE
        // Use NEON to check if any 32-bit lane is non-zero
        int32x4_t va = vld1q_s32(simd.i32);
        // OR the low and high halves together
        uint32x2_t reduced = vorr_u32(
            vreinterpret_u32_s32(vget_low_s32(va)),
            vreinterpret_u32_s32(vget_high_s32(va)));
        // Pairwise max to collapse to a single lane
        return (vget_lane_u32(vpmax_u32(reduced, reduced), 0) != 0);
#else
        return simd.i32[SIMD_X] || simd.i32[SIMD_Y] || simd.i32[SIMD_Z] || simd.i32[SIMD_W];
#endif
    }

    template <typename T>
    bool SIMDBool32x4Operation::OpAllTrue(const SIMDValue& val)
    {
        SIMDValue simd = SIMDUtils::CanonicalizeToBools<T>(val); //copy-by-value since we need to modify the copy

#if CHAKRA_NEON_AVAILABLE
        // Use NEON to check if all 32-bit lanes are non-zero
        int32x4_t va = vld1q_s32(simd.i32);
        // AND the low and high halves together
        uint32x2_t reduced = vand_u32(
            vreinterpret_u32_s32(vget_low_s32(va)),
            vreinterpret_u32_s32(vget_high_s32(va)));
        // Pairwise min to collapse — result is non-zero only if all lanes are non-zero
        return (vget_lane_u32(vpmin_u32(reduced, reduced), 0) != 0);
#else
        return simd.i32[SIMD_X] && simd.i32[SIMD_Y] && simd.i32[SIMD_Z] && simd.i32[SIMD_W];
#endif
    }

    template bool SIMDBool32x4Operation::OpAllTrue<int32>(const SIMDValue& val);
    template bool SIMDBool32x4Operation::OpAllTrue<int16>(const SIMDValue& val);
    template bool SIMDBool32x4Operation::OpAllTrue<int8>(const SIMDValue& val);
    //
    template bool SIMDBool32x4Operation::OpAnyTrue<int32>(const SIMDValue& val);
    template bool SIMDBool32x4Operation::OpAnyTrue<int16>(const SIMDValue& val);
    template bool SIMDBool32x4Operation::OpAnyTrue<int8>(const SIMDValue& val);
}
#endif