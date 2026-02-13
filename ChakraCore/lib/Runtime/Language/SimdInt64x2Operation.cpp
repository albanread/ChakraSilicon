//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#include "NeonAccel.h"

namespace Js
{
    SIMDValue SIMDInt64x2Operation::OpSplat(int64 val)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        vst1q_s64(reinterpret_cast<int64_t*>(result.i64), vdupq_n_s64(static_cast<int64_t>(val)));
#else
        result.i64[0] = val;
        result.i64[1] = val;
#endif
        return result;
    }

    SIMDValue SIMDInt64x2Operation::OpAdd(const SIMDValue& a, const SIMDValue& b)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int64x2_t va = vld1q_s64(reinterpret_cast<const int64_t*>(a.i64));
        int64x2_t vb = vld1q_s64(reinterpret_cast<const int64_t*>(b.i64));
        vst1q_s64(reinterpret_cast<int64_t*>(result.i64), vaddq_s64(va, vb));
#else
        result.i64[0] = a.i64[0] + b.i64[0];
        result.i64[1] = a.i64[1] + b.i64[1];
#endif
        return result;
    }

    SIMDValue SIMDInt64x2Operation::OpSub(const SIMDValue& a, const SIMDValue& b)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int64x2_t va = vld1q_s64(reinterpret_cast<const int64_t*>(a.i64));
        int64x2_t vb = vld1q_s64(reinterpret_cast<const int64_t*>(b.i64));
        vst1q_s64(reinterpret_cast<int64_t*>(result.i64), vsubq_s64(va, vb));
#else
        result.i64[0] = a.i64[0] - b.i64[0];
        result.i64[1] = a.i64[1] - b.i64[1];
#endif
        return result;
    }

    SIMDValue SIMDInt64x2Operation::OpNeg(const SIMDValue& a)
    {
        SIMDValue result;
#if CHAKRA_NEON_AVAILABLE
        int64x2_t va = vld1q_s64(reinterpret_cast<const int64_t*>(a.i64));
        vst1q_s64(reinterpret_cast<int64_t*>(result.i64), vnegq_s64(va));
#else
        result.i64[0] = -a.i64[0];
        result.i64[1] = -a.i64[1];
#endif
        return result;
    }

    static bool IsInRange(double val, uint64& out)
    {
        if (val != val || val <= (double)0)
        {
            out = 0;
            return false;
        }

        if (val >= (double)ULLONG_MAX)
        {
            out = ULLONG_MAX;
            return false;
        }

        return true;
    }

    static bool IsInRange(double val, int64& out)
    {
        if (val != val)
        {
            out = 0;
            return false;
        }

        if (val <= (double)LLONG_MIN)
        {
            out = LLONG_MIN;
            return false;
        }

        if (val >= (double)LLONG_MAX)
        {
            out = LLONG_MAX;
            return false;
        }

        return true;
    }

    template<typename T>
    void SIMDInt64x2Operation::OpTrunc(SIMDValue* dst, SIMDValue* src)
    {
        // OpTrunc has range-clamping semantics that don't map cleanly to a single
        // NEON instruction, so we keep the scalar implementation for correctness.
        T convertedVal;
        dst->i64[0] = IsInRange(src->f64[0], convertedVal) ? (T)src->f64[0] : convertedVal;
        dst->i64[1] = IsInRange(src->f64[1], convertedVal) ? (T)src->f64[1] : convertedVal;
    }

    template void SIMDInt64x2Operation::OpTrunc<int64>(SIMDValue* dst, SIMDValue* src);
    template void SIMDInt64x2Operation::OpTrunc<uint64>(SIMDValue* dst, SIMDValue* src);

    void SIMDInt64x2Operation::OpShiftLeftByScalar(SIMDValue* dst, SIMDValue* src, int count)
    {
        count = count & SIMDUtils::SIMDGetShiftAmountMask(8);
#if CHAKRA_NEON_AVAILABLE
        int64x2_t va = vld1q_s64(reinterpret_cast<const int64_t*>(src->i64));
        int64x2_t vcount = vdupq_n_s64(static_cast<int64_t>(count));
        vst1q_s64(reinterpret_cast<int64_t*>(dst->i64), vshlq_s64(va, vcount));
#else
        dst->i64[0] = src->i64[0] << count;
        dst->i64[1] = src->i64[1] << count;
#endif
    }

    void SIMDInt64x2Operation::OpShiftRightByScalar(SIMDValue* dst, SIMDValue* src, int count)
    {
        count = count & SIMDUtils::SIMDGetShiftAmountMask(8);
#if CHAKRA_NEON_AVAILABLE
        int64x2_t va = vld1q_s64(reinterpret_cast<const int64_t*>(src->i64));
        // Negative shift amount = arithmetic right shift in vshlq
        int64x2_t vcount = vdupq_n_s64(static_cast<int64_t>(-count));
        vst1q_s64(reinterpret_cast<int64_t*>(dst->i64), vshlq_s64(va, vcount));
#else
        dst->i64[0] = src->i64[0] >> count;
        dst->i64[1] = src->i64[1] >> count;
#endif
    }

    void SIMDInt64x2Operation::OpShiftRightByScalarU(SIMDValue* dst, SIMDValue* src, int count)
    {
        count = count & SIMDUtils::SIMDGetShiftAmountMask(8);
#if CHAKRA_NEON_AVAILABLE
        uint64x2_t va = vld1q_u64(reinterpret_cast<const uint64_t*>(src->i64));
        // Negative shift amount = logical right shift for unsigned in vshlq_u64
        int64x2_t vcount = vdupq_n_s64(static_cast<int64_t>(-count));
        vst1q_u64(reinterpret_cast<uint64_t*>(dst->i64), vshlq_u64(va, vcount));
#else
        dst->i64[0] = (uint64)src->i64[0] >> count;
        dst->i64[1] = (uint64)src->i64[1] >> count;
#endif
    }

    void SIMDInt64x2Operation::OpReplaceLane(SIMDValue* dst, SIMDValue* src, int64 val, uint index)
    {
#if CHAKRA_NEON_AVAILABLE
        // vsetq_lane requires a compile-time constant lane index,
        // so we branch on the runtime index (which can only be 0 or 1 for i64x2).
        int64x2_t v = vld1q_s64(reinterpret_cast<const int64_t*>(src->i64));
        if (index == 0)
        {
            v = vsetq_lane_s64(static_cast<int64_t>(val), v, 0);
        }
        else
        {
            v = vsetq_lane_s64(static_cast<int64_t>(val), v, 1);
        }
        vst1q_s64(reinterpret_cast<int64_t*>(dst->i64), v);
#else
        dst->SetValue(*src);
        dst->i64[index] = val;
#endif
    }
}