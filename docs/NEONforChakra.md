# NEON for ChakraSilicon — Acceleration Plan

**Date:** June 2025  
**Status:** Proposal  
**Target:** Apple Silicon (ARM64 with NEON)

---

## Executive Summary

ChakraSilicon currently makes **zero** use of NEON vector instructions. Every array operation — fill, copy, search, compare, reverse, sort — executes one scalar element at a time. Apple Silicon provides 32 × 128-bit NEON registers capable of processing 4 floats, 2 doubles, 4 int32s, 8 int16s, or 16 int8s per cycle. This document lays out a phased plan to exploit NEON across the engine, from low-hanging runtime intrinsics to JIT auto-vectorization.

---

## Table of Contents

1. [Current State](#1-current-state)
2. [Phase 1: C++ Runtime NEON Intrinsics](#2-phase-1-c-runtime-neon-intrinsics)
3. [Phase 2: JIT Helper Acceleration](#3-phase-2-jit-helper-acceleration)
4. [Phase 3: JIT NEON Code Generation](#4-phase-3-jit-neon-code-generation)
5. [Phase 4: Auto-Vectorization](#5-phase-4-auto-vectorization)
6. [Phase 5: WASM SIMD on ARM64](#6-phase-5-wasm-simd-on-arm64)
7. [Architecture Reference](#7-architecture-reference)
8. [Testing Strategy](#8-testing-strategy)
9. [Risk Assessment](#9-risk-assessment)
10. [Priority Matrix](#10-priority-matrix)

---

## 1. Current State

### What Exists

| Component | Location | Status |
|-----------|----------|--------|
| NEON register definitions (D0–D29) | `lib/Backend/arm64/RegList.h` | ✅ Present, used for scalar FP only |
| NEON encoder infrastructure | `lib/Backend/arm64/ARM64NeonEncoder.h` | ✅ Present, mostly unused |
| ARM64 MdOpCodes | `lib/Backend/arm64/MdOpCodes.h` | ⚠️ Scalar FP only (FADD, FMUL, etc.) — no vector ops |
| Scalar SIMD operation fallbacks | `lib/Runtime/Language/SimdFloat32x4Operation.cpp` etc. | ✅ Compiled on ARM64, element-by-element C code |
| x86 SSE SIMD lowering | `lib/Backend/LowerMDSharedSimd128.cpp` | ❌ x86 only, gated by `ENABLE_WASM_SIMD` |
| `ASMJS_PLAT` / `ENABLE_WASM_SIMD` | `lib/Common/CommonDefines.h` | ❌ Requires `_M_IX86 || _M_X64`, ARM64 excluded |
| MemOp JIT optimization | `lib/Backend/GlobOpt.cpp` (CollectMemOpInfo) | ⚠️ Detects loops, emits C helper calls — no vectorization |

### What Does Not Exist

- No NEON vector instructions in `MdOpCodes.h` (no `LD1`, `ST1`, `DUP`, `FADDP`, `CMEQ`, etc.)
- No 128-bit register type (`TySimd128`) allocation on ARM64
- No NEON lowering pass in the ARM64 JIT backend
- No `<arm_neon.h>` intrinsics anywhere in the C++ runtime
- No auto-vectorization of any kind in the JIT compiler

### Scalar Code That Should Be Vectorized

Every one of these is a tight loop over contiguous typed memory:

| Function | File | What It Does |
|----------|------|-------------|
| `CopyValueToSegmentBuferNoCheck<double>` | `JavascriptArray.cpp:4323` | Fill double array — scalar loop |
| `CopyValueToSegmentBuferNoCheck<int32>` | `JavascriptArray.cpp:4339` | Fill int32 array — scalar loop |
| `CopyValueToSegmentBuferNoCheck<Var>` | `JavascriptArray.cpp:4355` | Fill Var array — scalar loop |
| `TypedArray::DirectSetItemAtRange` | `TypedArray.h:390` | TypedArray `.fill()` — scalar loop for non-zero |
| `TypedArray::DirectSetItemAtRange` (copy variant) | `TypedArray.h:338` | TypedArray `.set()` — `memcpy` or scalar |
| `TypedArray::Set` | `TypedArray.cpp:844` | Cross-type set — element-by-element |
| `TypedArray::FindMinOrMax` | `TypedArray.cpp:2532` | Min/max scan — scalar loop |
| `HeadSegmentIndexOfHelper` | `JavascriptArray.cpp:4259` | `indexOf`/`includes` — scalar scan |
| `JavascriptNativeIntArray::HeadSegmentIndexOfHelper` | `JavascriptArray.cpp:4362` | Int array search — scalar scan |
| `JavascriptNativeFloatArray::HeadSegmentIndexOfHelper` | `JavascriptArray.cpp:4418` | Float array search — scalar scan |
| `SimdFloat32x4Operation::OpAdd` (ARM path) | `SimdFloat32x4Operation.cpp` | 4-lane float add — 4 scalar adds |
| `SimdInt32x4Operation::OpAdd` (ARM path) | `SimdInt32x4Operation.cpp` | 4-lane int add — 4 scalar adds |
| `OP_Memset` helper | `JavascriptOperators.cpp:5192` | JIT memset — dispatches to scalar loops |
| `OP_Memcopy` helper | `JavascriptOperators.cpp:5074` | JIT memcopy — dispatches to scalar loops |

---

## 2. Phase 1: C++ Runtime NEON Intrinsics

**Effort:** Medium  
**Impact:** High — benefits all execution modes (interpreter, SimpleJit, FullJit)  
**Risk:** Low — no JIT changes, pure C++ with `<arm_neon.h>`  
**Dependencies:** None

### 2.1 New Header: `NeonAccel.h`

Create `lib/Runtime/Language/NeonAccel.h` — a header-only NEON utility library with inlined operations, gated on `defined(_M_ARM64)` or `defined(__aarch64__)`.

```c++
#pragma once
#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define CHAKRA_NEON_AVAILABLE 1
#else
#define CHAKRA_NEON_AVAILABLE 0
#endif
```

### 2.2 TypedArray Fill (`DirectSetItemAtRange`)

**File:** `lib/Runtime/Library/TypedArray.h`  
**Current code (line ~430):**
```c++
for (uint32 i = 0; i < newLength; i++) {
    typedBuffer[newStart + i] = typedValue;
}
```

**NEON replacement strategy by element type:**

| TypedArray | NEON Approach | Elements/Cycle |
|------------|-------------|----------------|
| `Float32Array` | `vdupq_n_f32` + `vst1q_f32` | 4 |
| `Float64Array` | `vdupq_n_f64` + `vst1q_f64` | 2 |
| `Int32Array` / `Uint32Array` | `vdupq_n_s32` + `vst1q_s32` | 4 |
| `Int16Array` / `Uint16Array` | `vdupq_n_s16` + `vst1q_s16` | 8 |
| `Int8Array` / `Uint8Array` | `vdupq_n_s8` + `vst1q_s8` | 16 |

**Pseudocode for Float32Array fill:**
```c++
void NeonFillFloat32(float* dst, uint32 count, float value) {
    float32x4_t vval = vdupq_n_f32(value);
    uint32 i = 0;
    // Main loop: 16 floats per iteration (4 stores of 4)
    for (; i + 15 < count; i += 16) {
        vst1q_f32(dst + i,      vval);
        vst1q_f32(dst + i + 4,  vval);
        vst1q_f32(dst + i + 8,  vval);
        vst1q_f32(dst + i + 12, vval);
    }
    // 4-element tail
    for (; i + 3 < count; i += 4) {
        vst1q_f32(dst + i, vval);
    }
    // Scalar tail
    for (; i < count; i++) {
        dst[i] = value;
    }
}
```

**Expected speedup:** 4–16× depending on element size.

### 2.3 TypedArray Search (`indexOf`, `includes`)

**File:** `lib/Runtime/Library/TypedArray.cpp` — `EntryIndexOf`, `EntryIncludes`  
**Also:** `JavascriptArray.cpp` — `HeadSegmentIndexOfHelper` variants

**NEON approach for Int32Array indexOf:**
```c++
int32 NeonIndexOfInt32(const int32* buf, uint32 len, int32 target) {
    int32x4_t vtarget = vdupq_n_s32(target);
    uint32 i = 0;
    for (; i + 3 < len; i += 4) {
        int32x4_t chunk = vld1q_s32(buf + i);
        uint32x4_t cmp = vceqq_s32(chunk, vtarget);
        // Check if any lane matched
        uint32x2_t reduced = vorr_u32(vget_low_u32(cmp), vget_high_u32(cmp));
        if (vget_lane_u32(vpmax_u32(reduced, reduced), 0) != 0) {
            // Find exact lane
            for (uint32 j = 0; j < 4 && (i + j) < len; j++) {
                if (buf[i + j] == target) return (int32)(i + j);
            }
        }
    }
    for (; i < len; i++) {
        if (buf[i] == target) return (int32)i;
    }
    return -1;
}
```

Equivalent strategies exist for `float` (`vceqq_f32`), `int16` (`vceqq_s16` — 8 lanes), and `int8` (`vceqq_s8` — 16 lanes).

**Expected speedup:** 4–16× depending on element size. Early-exit on first match preserves best-case O(1).

### 2.4 Min/Max Scan (`FindMinOrMax`)

**File:** `lib/Runtime/Library/TypedArray.cpp:2532`

**Current:** Scalar loop with NaN checks per element.

**NEON approach for Float32Array:**
```c++
// First pass: NEON scan for extremum (ignoring NaN)
float32x4_t vmin = vld1q_f32(buf);
for (i = 4; i + 3 < len; i += 4) {
    float32x4_t chunk = vld1q_f32(buf + i);
    vmin = vminq_f32(vmin, chunk);  // FMIN handles NaN propagation per IEEE 754-2019
}
// Horizontal reduce
float32x2_t half = vpmin_f32(vget_low_f32(vmin), vget_high_f32(vmin));
half = vpmin_f32(half, half);
float result = vget_lane_f32(half, 0);
```

**Note:** `vminq_f32` / `vmaxq_f32` propagate NaN per ARM semantics which aligns with the ECMAScript requirement ("If any value is NaN, return NaN"). Must verify NaN handling matches spec for `-0` vs `+0` edge cases.

**Expected speedup:** ~4× for float32, ~2× for float64.

### 2.5 Array Reverse

**File:** `lib/Runtime/Library/TypedArray.cpp:2098` → delegates to `JavascriptArray::ReverseHelper`

**NEON approach:** Load 4 elements from front, load 4 from back, `vrev64q` + swap halves, store crossed.

```c++
void NeonReverseFloat32(float* buf, uint32 len) {
    uint32 lo = 0, hi = len - 4;
    while (lo + 3 < hi) {
        float32x4_t front = vld1q_f32(buf + lo);
        float32x4_t back  = vld1q_f32(buf + hi);
        // Reverse each 4-element chunk
        front = vrev64q_f32(front);
        front = vcombine_f32(vget_high_f32(front), vget_low_f32(front));
        back  = vrev64q_f32(back);
        back  = vcombine_f32(vget_high_f32(back), vget_low_f32(back));
        vst1q_f32(buf + lo, back);
        vst1q_f32(buf + hi, front);
        lo += 4; hi -= 4;
    }
    // Scalar middle
}
```

### 2.6 SIMD Operation Backends (SimdXxxOperation)

**Files:** All `Simd*Operation.cpp` files (12 files)

Currently the ARM path uses scalar C code:
```c++
// SimdFloat32x4Operation.cpp, ARM path
result.f32[SIMD_X] = aValue.f32[SIMD_X] + bValue.f32[SIMD_X];
result.f32[SIMD_Y] = aValue.f32[SIMD_Y] + bValue.f32[SIMD_Y];
result.f32[SIMD_Z] = aValue.f32[SIMD_Z] + bValue.f32[SIMD_Z];
result.f32[SIMD_W] = aValue.f32[SIMD_W] + bValue.f32[SIMD_W];
```

**Replace with:**
```c++
SIMDValue SIMDFloat32x4Operation::OpAdd(const SIMDValue& a, const SIMDValue& b) {
    SIMDValue result;
    float32x4_t va = vld1q_f32(a.f32);
    float32x4_t vb = vld1q_f32(b.f32);
    vst1q_f32(result.f32, vaddq_f32(va, vb));
    return result;
}
```

**Operations to port per type:**

| Category | Operations | NEON Intrinsics |
|----------|-----------|-----------------|
| Arithmetic | Add, Sub, Mul, Div | `vaddq`, `vsubq`, `vmulq`, `vdivq` (f32/f64 only for div) |
| Bitwise | And, Or, Xor, Not | `vandq`, `vorrq`, `veorq`, `vmvnq` |
| Compare | Eq, Ne, Lt, Le, Gt, Ge | `vceqq`, `vcltq`, `vcleq`, `vcgtq`, `vcgeq` |
| Math | Abs, Neg, Sqrt, Min, Max | `vabsq`, `vnegq`, `vsqrtq`, `vminq`, `vmaxq` |
| Conversion | FromInt32x4, FromFloat32x4 | `vcvtq_f32_s32`, `vcvtq_s32_f32` |
| Shuffle | Splat, Swizzle | `vdupq_n`, `vqtbl1q` |
| Shift | ShiftLeft, ShiftRight | `vshlq`, `vshrq` |
| Select | BitSelect | `vbslq` |

This covers all 12 files × ~15 operations each ≈ **~180 function replacements** but each is a trivial 3–5 line rewrite.

**Expected speedup:** 4× for most operations (they go from 4 scalar ops to 1 vector op).

### 2.7 Bulk Memory Helpers

**Files:**
- `JavascriptArray.cpp` — `CopyValueToSegmentBuferNoCheck`
- `JavascriptOperators.cpp` — `OP_Memset`, `OP_Memcopy`

The non-zero fill paths should use NEON stores. The copy paths already delegate to `memmove_s` / `memcpy_s` for same-type cases, which Apple's libc implements with NEON. The improvement here is for the **cross-type copy** and **non-trivial fill** paths.

### 2.8 Implementation Checklist — Phase 1

- [ ] Create `NeonAccel.h` with compile-time guards and helper macros
- [ ] Add NEON fill paths for all TypedArray element types (8 types)
- [ ] Add NEON indexOf/includes for Int32Array, Float32Array, Int8Array
- [ ] Add NEON min/max scan for Float32Array, Float64Array
- [ ] Add NEON reverse for all TypedArray element types
- [ ] Replace all 12 `SimdXxxOperation.cpp` ARM scalar paths with NEON intrinsics
- [ ] Add NEON fill path to `CopyValueToSegmentBuferNoCheck` for int32/double
- [ ] Unit tests for each accelerated path
- [ ] Benchmark harness comparing scalar vs NEON

---

## 3. Phase 2: JIT Helper Acceleration

**Effort:** Medium  
**Impact:** High — MemOp is the JIT's main loop optimization  
**Risk:** Low–Medium  
**Dependencies:** Phase 1 (NeonAccel.h)

### 3.1 Problem

The JIT's MemOp optimization (`GlobOpt::CollectMemOpInfo` → `GlobOpt::EmitMemop`) detects simple fill/copy loops and replaces them with `Js::OpCode::Memset` / `Js::OpCode::Memcopy`. These lower to C helper calls (`HelperOp_Memset`, `HelperOp_Memcopy`) in `Lower.cpp:1564`.

The helpers (`OP_Memset`, `OP_Memcopy` in `JavascriptOperators.cpp`) then dispatch to scalar `DirectSetItemAtRange` / `DirectSetItemAtRangeFromArray`.

### 3.2 Solution

Make the helpers type-aware and dispatch to NEON-accelerated paths:

```c++
BOOL JavascriptOperators::OP_Memset(Var instance, int32 start, Var value, int32 length, ScriptContext* scriptContext) {
    // ... existing type dispatch ...
    case TypeIds_Float32Array:
    {
        float typedValue = JavascriptConversion::ToFloat(value, scriptContext);
#if CHAKRA_NEON_AVAILABLE
        NeonFillFloat32(((Float32Array*)instance)->DirectBuffer() + start, length, typedValue);
        returnValue = TRUE;
#else
        returnValue = VarTo<Float32Array>(instance)->DirectSetItemAtRange(start, length, typedValue);
#endif
        break;
    }
}
```

### 3.3 NEON-Accelerated OP_Memcopy for Same-Type

When source and destination are the same typed array type and don't overlap, use NEON bulk copy (LD1/ST1 pairs) instead of scalar element copy. For overlapping regions, reverse-direction NEON copy.

### 3.4 Implementation Checklist — Phase 2

- [ ] Modify `OP_Memset` to dispatch to NEON fill per TypedArray type
- [ ] Modify `OP_Memcopy` to use NEON load/store for same-type copies
- [ ] Add NEON-aware `DirectSetItemAtRange` specializations for each TypedArray type
- [ ] Benchmark MemOp loops (e.g., `for(i=0;i<N;i++) arr[i]=42.0`) before/after

---

## 4. Phase 3: JIT NEON Code Generation

**Effort:** Medium (reduced from High — borrowed-register strategy eliminates allocator rework)  
**Impact:** Very High — enables native NEON in JIT-compiled code  
**Risk:** Low (reduced from Medium–High — no changes to linear scan allocator)  
**Dependencies:** None (can be done in parallel with Phase 1–2)

### 4.1 Phase Flag: `NeonSimd`

All JIT NEON codegen is gated behind a Chakra phase flag, following the same pattern as `MemOp`. NEON is restricted to **FullJit only** (level 2) — SimpleJit never emits NEON instructions. This ensures the profiling interpreter and SimpleJit have collected sufficient type and loop-count data before NEON sequences are emitted.

**Phase declaration in `lib/Common/ConfigFlagsList.h`:**

Place under the existing `MemOp` sub-tree inside `GlobOpt`:

```
                PHASE(MemOp)
                    PHASE(MemSet)
                    PHASE(MemCopy)
                PHASE(NeonSimd)              // NEW — master gate for all JIT NEON codegen
                    PHASE(NeonVectorize)     // NEW — auto-vectorization of hot loops (Phase 4)
```

This gives us `-off:NeonSimd` to disable all NEON codegen, `-trace:NeonSimd` for diagnostics, and `-off:NeonVectorize` to disable auto-vectorization independently.

**Minimum loop count flag in `lib/Common/ConfigFlagsList.h`:**

```
#define DEFAULT_CONFIG_MinNeonSimdLoopCount (16U)
```

```
FLAGNRA(Number, MinNeonSimdLoopCount, Mnslc, "Minimum iteration count of a loop to activate NEON vectorization", DEFAULT_CONFIG_MinNeonSimdLoopCount)
```

This mirrors `MinMemOpCount` (also defaulting to 16). Loops that execute fewer than 16 iterations are not worth vectorizing — the NEON setup overhead (DUP splat, scalar epilogue) would exceed the savings.

**Gating helper in `lib/Backend/Func.h`:**

```c++
bool DoNeonSimd() const
{
    return
        !IsSimpleJit() &&
        DoGlobOpt() &&
        !PHASE_OFF(Js::NeonSimdPhase, this);
}

bool DoNeonVectorize() const
{
    return
        DoNeonSimd() &&
        !PHASE_OFF(Js::NeonVectorizePhase, this);
}
```

`DoNeonSimd()` gates Phase 3 (explicit SIMD IR lowering to NEON). `DoNeonVectorize()` gates Phase 4 (auto-vectorization). Both require FullJit — SimpleJit skips them entirely. The `DoGlobOpt()` requirement ensures type specialization and induction variable analysis have run before NEON lowering attempts to use their results.

**Trace support in `lib/Backend/arm64/LowerMDSimd128.cpp`:**

```c++
#define DO_NEONSIMD_TRACE() (PHASE_TRACE(Js::NeonSimdPhase, this->func))
#define DO_NEONSIMD_TRACE_VERBOSE() (DO_NEONSIMD_TRACE() && CONFIG_FLAG(Verbose))

#define TRACE_NEONSIMD(loop, instr, ...) \
    if (DO_NEONSIMD_TRACE()) { \
        Output::Print(_u("NeonSimd: ")); \
        Output::Print(__VA_ARGS__); \
        Output::Print(_u("\n")); \
        Output::Flush(); \
    }
```

This follows the `DO_MEMOP_TRACE()` pattern and enables `-trace:NeonSimd` for diagnosing which loops get vectorized and why.

**Usage in the lowerer:**

```c++
// In LowerMD.cpp — SIMD IR dispatch
if (func->DoNeonSimd())
{
    // Lower SIMD IR opcodes to NEON MdOpcodes using borrowed Q27/Q28/Q29
    LowerSimd128Instruction(instr);
}
else
{
    // Fall through to scalar lowering or helper call
}

// In GlobOpt.cpp — auto-vectorization candidate collection (Phase 4)
if (!PHASE_OFF(Js::NeonSimdPhase, this->func) &&
    !PHASE_OFF(Js::NeonVectorizePhase, this->func) &&
    loop->loopCount &&
    loop->loopCount->IsConst() &&
    loop->loopCount->GetConst() >= CONFIG_FLAG(MinNeonSimdLoopCount))
{
    CollectNeonVectorizeInfo(instrPrev, instr, src1Val, src2Val);
}
```

**Why FullJit only:**

| JIT Tier | Profile Data | Type Specialization | Induction Vars | NEON? |
|----------|-------------|--------------------|-|-------|
| Interpreter | Collecting | ✗ | ✗ | ✗ |
| SimpleJit | Available but unused | ✗ | ✗ | ✗ |
| FullJit | Used by GlobOpt | ✅ | ✅ | ✅ |

SimpleJit does not run GlobOpt, does not type-specialize, and does not analyze loops. Emitting NEON in SimpleJit would require the lowerer to make unsafe assumptions about TypedArray element types and loop bounds. FullJit has already proven these via GlobOpt's value tracking and bailout insertion, making NEON emission safe.

**Hot loop focus:** The `MinNeonSimdLoopCount` threshold (default 16) ensures only loops with enough iterations to amortize NEON overhead are candidates. For a `float32x4` operation processing 4 elements per NEON instruction, 16 iterations = 64 elements — the minimum where vectorization reliably outperforms scalar. The threshold is configurable: `-MinNeonSimdLoopCount:64` for conservative use, or `-MinNeonSimdLoopCount:4` for aggressive testing.

### 4.2 Register Strategy: Borrowed NEON Scratch Registers

**Decision:** Do **not** modify the register allocator for 128-bit Q-register support. Instead, reserve D27/D28/D29 as dedicated NEON scratch registers (accessed as Q27/Q28/Q29 for 128-bit operations).

**Why full Q-register integration is impractical now:**

The linear scan register allocator tracks all registers via a 64-bit `BitVector` (`BVUnit64` in `Backend.h`). ARM64 currently has exactly 64 register entries (1 NOREG + 33 integer + 30 float), filling the bitvector completely — D30/D31 are already commented out in `RegList.h` because they would overflow it. Adding Q0–Q29 as separate allocatable entries would require:

1. Expanding `BitVector` to 128 bits — touching every set operation in `LinearScan.cpp` (`FindReg`, `Spill`, `KillImplicitRegs`, `CheckInvariants`, `InsertSecondChanceCompensation`, `ReconcileRegContent`, etc.)
2. Register aliasing logic — Q0 and D0 are the same physical register, so allocating one must block the other. Chakra's allocator has zero infrastructure for overlapping register classes.
3. Updating `EnsureSpillSymForVFPReg` — currently hardcoded to `TyFloat64` / 8-byte spill slots; Q registers need 16-byte `TySimd128` slots.
4. Updating bailout save/restore — `RegisterSaveSlotCount`, `GetRegisterSaveIndex`, `SaveAllRegistersAndBailOut` all assume the current flat layout.

This would be a 4–6 week effort in the most fragile part of the backend with high regression risk.

**The borrowed-register approach:**

**File:** `lib/Backend/arm64/RegList.h`

```
// Before:
REGDAT(D27,     d27,      NEONREG_D27,   TyFloat64,  0)
REGDAT(D28,     d28,      NEONREG_D28,   TyFloat64,  0)
REGDAT(D29,     d29,      NEONREG_D29,   TyFloat64,  0)

// After:
REGDAT(D27,     d27,      NEONREG_D27,   TyFloat64,  RA_DONTALLOCATE)  // Reserved: NEON scratch Q27
REGDAT(D28,     d28,      NEONREG_D28,   TyFloat64,  RA_DONTALLOCATE)  // Reserved: NEON scratch Q28
REGDAT(D29,     d29,      NEONREG_D29,   TyFloat64,  RA_DONTALLOCATE)  // Reserved: NEON scratch Q29
```

**New file:** `lib/Backend/arm64/NeonRegs.h`

```c++
#pragma once

// Dedicated NEON 128-bit scratch registers.
// These are borrowed from the top of the FP register file by marking
// D27–D29 as RA_DONTALLOCATE in RegList.h. The lowerer and encoder
// reference them as Q27–Q29 for 128-bit NEON operations.
//
// The register allocator is completely unaware of these — they are
// managed explicitly by the NEON lowering pass, analogous to how
// SCRATCH_REG (R17) is used for address materialization.

#define NEON_SCRATCH_Q0   27   // Q27 / V27 — first NEON scratch
#define NEON_SCRATCH_Q1   28   // Q28 / V28 — second NEON scratch
#define NEON_SCRATCH_Q2   29   // Q29 / V29 — third NEON scratch (destination)

// Usage convention in lowered NEON sequences:
//   Q27 — source A (or accumulator)
//   Q28 — source B
//   Q29 — destination / result
//
// For two-operand patterns (e.g., reductions), Q28 is free as a temp.
// For one-operand patterns (e.g., DUP+store fill), Q28 and Q29 are free.
```

**What this costs:**

| Concern | Impact |
|---------|--------|
| Allocatable FP registers | 27 remaining (D0–D26) — ample for JS workloads |
| BitVector size | Unchanged — still 64 bits |
| Register aliasing | Non-issue — reserved regs are never in the active set |
| Spill slot sizing | Non-issue — allocator never spills these |
| Bailout save/restore | No change needed |
| Prolog/epilog | No change — D27–D29 are caller-saved, no save/restore obligation |
| LinearScanMD.cpp | No change needed |

**Three registers is sufficient for all targeted patterns:**

| Pattern | Q27 | Q28 | Q29 | Fits? |
|---------|-----|-----|-----|-------|
| Element-wise binary (`c[i] = a[i] + b[i]`) | src A | src B | dest | ✅ |
| Element-wise unary (`b[i] = -a[i]`) | src | — | dest | ✅ |
| Scalar broadcast + op (`b[i] = a[i] * k`) | src | splat | dest | ✅ |
| Reduction (`sum += a[i]`) | accum | chunk | — | ✅ |
| Compare + select (`FCMGT` + `BSL`) | src A | src B | mask→dest | ✅ (sequential reuse) |
| Fill (`memset`-style) | splat | — | — | ✅ |

**Future path:** If WASM SIMD or complex auto-vectorization demands more than 3 live 128-bit values, the full allocator integration can be revisited. The borrowed-register approach is designed to be replaced transparently — the lowerer's explicit register assignments would simply become allocator-managed operands.

### 4.3 New NEON Instructions in MdOpCodes.h

**File:** `lib/Backend/arm64/MdOpCodes.h`

Add vector instruction entries. Minimum viable set:

```
// NEON Vector Load/Store
MACRO(LD1_4S,     Reg2,    0,  UNUSED, LEGAL_LOAD,  UNUSED, DL__)   // LD1 {Vt.4S}, [Xn]
MACRO(ST1_4S,     Reg2,    0,  UNUSED, LEGAL_STORE, UNUSED, DS__)   // ST1 {Vt.4S}, [Xn]
MACRO(LD1_2D,     Reg2,    0,  UNUSED, LEGAL_LOAD,  UNUSED, DL__)   // LD1 {Vt.2D}, [Xn]
MACRO(ST1_2D,     Reg2,    0,  UNUSED, LEGAL_STORE, UNUSED, DS__)   // ST1 {Vt.2D}, [Xn]

// NEON Vector Arithmetic (float32x4)
MACRO(VADD_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)   // FADD Vd.4S, Vn.4S, Vm.4S
MACRO(VSUB_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VMUL_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VDIV_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)

// NEON Vector Arithmetic (int32x4)
MACRO(VADD_I4S,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VSUB_I4S,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VMUL_I4S,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)

// NEON Compare
MACRO(VCEQ_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)   // CMEQ Vd.4S, Vn.4S, Vm.4S
MACRO(VCGT_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VCLT_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)

// NEON Bitwise
MACRO(VAND_16B,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VORR_16B,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VEOR_16B,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VBSL_16B,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)   // Bit select

// NEON Splat
MACRO(VDUP_4S,    Reg2,    0,  UNUSED, LEGAL_REG2,  UNUSED, D___)   // DUP Vd.4S, Wn
MACRO(VDUP_2D,    Reg2,    0,  UNUSED, LEGAL_REG2,  UNUSED, D___)   // DUP Vd.2D, Xn

// NEON Shuffle
MACRO(VTBL_16B,   Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)   // TBL Vd.16B, {Vn.16B}, Vm.16B

// NEON Min/Max
MACRO(VMIN_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)
MACRO(VMAX_4S,    Reg3,    0,  UNUSED, LEGAL_REG3,  UNUSED, D___)

// NEON Convert
MACRO(VCVT_F4S,   Reg2,    0,  UNUSED, LEGAL_REG2,  UNUSED, D___)   // SCVTF Vd.4S, Vn.4S
MACRO(VCVT_I4S,   Reg2,    0,  UNUSED, LEGAL_REG2,  UNUSED, D___)   // FCVTZS Vd.4S, Vn.4S

// NEON Reduction
MACRO(VADDV_4S,   Reg2,    0,  UNUSED, LEGAL_REG2,  UNUSED, D___)   // ADDV Sd, Vn.4S
MACRO(VUMAXV_16B, Reg2,    0,  UNUSED, LEGAL_REG2,  UNUSED, D___)   // UMAXV Bd, Vn.16B
```

### 4.4 Encoder Changes

**File:** `lib/Backend/arm64/EncoderMD.cpp`

Add encoding cases for each new NEON opcode. The `ARM64NeonEncoder.h` already has the encoding infrastructure — it needs to be wired up for the new opcodes.

Key encoding patterns:
- **LD1/ST1:** `0 Q 001100 L 10 0000 0111 sz Rn Rt` (single structure, no offset)
- **FADD (vector):** `0 Q 0 01110 0 sz 1 Rm 11010 1 Rn Rd`
- **ADD (vector int):** `0 Q 0 01110 sz 1 Rm 10000 1 Rn Rd`
- **DUP (general):** `0 Q 0 01110000 imm5 0 0001 1 Rn Rd`
- **CMEQ:** `0 Q 1 01110 sz 1 Rm 10001 1 Rn Rd`

### 4.5 Lowering: SIMD IR → NEON MdOpcodes

**New file:** `lib/Backend/arm64/LowerMDSimd128.cpp`

This is the ARM64 equivalent of `LowerMDSharedSimd128.cpp` (which is x86-only). It maps Chakra's internal SIMD IR opcodes to the new ARM64 NEON MdOpcodes.

Because the register allocator does not manage Q registers, the lowerer emits NEON sequences with **explicit, fixed register assignments** using the three borrowed scratch registers (Q27/Q28/Q29). This is analogous to how `SCRATCH_REG` (R17) is used elsewhere in the ARM64 backend.

```c++
#include "NeonRegs.h"

// Opcode mapping table (subset)
Js::OpCode simd128OpcodeMapNeon[] = {
    [Simd128_Add_F4]  = Js::OpCode::VADD_4S,
    [Simd128_Sub_F4]  = Js::OpCode::VSUB_4S,
    [Simd128_Mul_F4]  = Js::OpCode::VMUL_4S,
    [Simd128_Add_I4]  = Js::OpCode::VADD_I4S,
    [Simd128_Sub_I4]  = Js::OpCode::VSUB_I4S,
    [Simd128_And_I4]  = Js::OpCode::VAND_16B,
    [Simd128_Or_I4]   = Js::OpCode::VORR_16B,
    [Simd128_LdArr_F4] = Js::OpCode::LD1_4S,
    [Simd128_StArr_F4] = Js::OpCode::ST1_4S,
    // ...
};

// Example: lowering Simd128_Add_F4 with borrowed registers
//
//   IR:   Simd128_Add_F4  dst, srcA, srcB
//
//   Emitted (all hard-coded to Q27/Q28/Q29):
//     LD1  {V27.4S}, [Xn]        // load srcA → Q27
//     LD1  {V28.4S}, [Xm]        // load srcB → Q28
//     FADD V29.4S, V27.4S, V28.4S // Q29 = Q27 + Q28
//     ST1  {V29.4S}, [Xd]        // store Q29 → dst
//
// The lowerer resolves srcA/srcB/dst to GP register indirs
// (via the normal allocator) and then uses the fixed Q regs
// for the NEON data path. No Q-register spill is ever needed
// because lifetimes do not cross instruction boundaries.
```

### 4.6 Implementation Checklist — Phase 3

- [ ] Add `PHASE(NeonSimd)` and `PHASE(NeonVectorize)` to `ConfigFlagsList.h` under `GlobOpt`
- [ ] Add `DEFAULT_CONFIG_MinNeonSimdLoopCount` (16) and `MinNeonSimdLoopCount` flag to `ConfigFlagsList.h`
- [ ] Add `DoNeonSimd()` and `DoNeonVectorize()` helpers to `Func.h` (FullJit-only gate)
- [ ] Mark D27–D29 as `RA_DONTALLOCATE` in `RegList.h`
- [ ] Create `NeonRegs.h` with scratch register definitions and usage convention
- [ ] Add minimum viable NEON opcodes to `MdOpCodes.h`
- [ ] Implement encoding for all new opcodes in `EncoderMD.cpp`
- [ ] Create `LowerMDSimd128.cpp` for ARM64 SIMD lowering (using fixed Q27/Q28/Q29)
- [ ] Gate `LowerSimd128Instruction()` dispatch on `func->DoNeonSimd()` in `LowerMD.cpp`
- [ ] Add `DO_NEONSIMD_TRACE()` macros and `-trace:NeonSimd` support
- [ ] Update `LegalizeMD.cpp` for NEON instruction legalization
- [ ] Verify no existing code paths depend on D27–D29 being allocatable
- [ ] Test with asm.js SIMD programs
- [ ] Test flag control: `-off:NeonSimd` disables all NEON, `-trace:NeonSimd` emits diagnostics
- [ ] Validate generated code with Capstone disassembly (`-TraceJitAsm`)
- [ ] Stress-test FP register pressure: confirm 27 D-registers (D0–D26) cause no regressions

---

## 5. Phase 4: Auto-Vectorization

**Effort:** Very High  
**Impact:** Transformative — user JS code automatically benefits from NEON  
**Risk:** High  
**Dependencies:** Phase 3 (JIT NEON codegen must work)

### 5.0 Gating: Hot Loops in FullJit Only

Auto-vectorization is gated by `func->DoNeonVectorize()` (which implies `DoNeonSimd()` and FullJit). The vectorization candidate collector runs inside `GlobOpt::OptInstr` — the same place `CollectMemOpInfo` runs — and is subject to the same FullJit-only, post-profile-collection guarantees.

**Hot loop identification** uses two signals already available in FullJit:

1. **Static loop count** — If the loop bound is a constant or provably tied to `TypedArray.length`, `loop->loopCount->GetConst()` gives the iteration count. Checked against `CONFIG_FLAG(MinNeonSimdLoopCount)` (default 16).
2. **Profile-based loop count** — For dynamic bounds, the profiling interpreter records how many times each loop body executed. FullJit has access to this via `loop->GetProfiledLoopCount()`. Loops that never reached the threshold in profiling are skipped.

Only loops that satisfy **both** the `NeonVectorize` phase gate **and** the minimum iteration threshold are vectorization candidates. This avoids wasting compile time analyzing cold or trivially short loops.

```c++
// In GlobOpt.cpp — vectorization candidate collection
if (func->DoNeonVectorize() &&
    !isHoisted &&
    this->currentBlock->loop &&
    !IsLoopPrePass() &&
    this->currentBlock->loop->doMemOp &&   // reuse MemOp's loop validity checks
    this->currentBlock->loop->loopCount &&
    this->currentBlock->loop->loopCount->ConstOrEstimatedCount() >= CONFIG_FLAG(MinNeonSimdLoopCount))
{
    CollectNeonVectorizeInfo(instrPrev, instr, src1Val, src2Val);
}
```

### 5.1 What Auto-Vectorization Means

The JIT would detect loops like:

```javascript
const a = new Float32Array(1024);
const b = new Float32Array(1024);
const c = new Float32Array(1024);
for (let i = 0; i < 1024; i++) {
    c[i] = a[i] + b[i];
}
```

And instead of emitting 1024 scalar `FADD s0, s1, s2` instructions, emit 256 vector `FADD v0.4s, v1.4s, v2.4s` instructions.

### 5.2 Required Analysis Passes

1. **Loop detection** — Already exists (`FlowGraph::BuildLoop`)
2. **Induction variable analysis** — Already exists (`CollectMemOpInfo`, `InductionVariable`)
3. **Dependence analysis** — **NEW** — Must prove no loop-carried dependencies
4. **TypedArray type inference** — Partially exists via value type tracking in GlobOpt
5. **Vectorization legality check** — **NEW** — Ensure all ops in the loop body are vectorizable
6. **Cost model** — **NEW** — Don't vectorize if overhead exceeds benefit; enforced by `MinNeonSimdLoopCount` threshold (default 16 iterations) and compile-time heuristics

### 5.3 Vectorizable Patterns

**Tier 1 — Trivial (no cross-iteration dependencies):**

| Pattern | Example | NEON |
|---------|---------|------|
| Element-wise binary | `c[i] = a[i] + b[i]` | `FADD v.4s` |
| Element-wise unary | `b[i] = -a[i]` | `FNEG v.4s` |
| Scalar broadcast | `b[i] = a[i] * scale` | `DUP` + `FMUL v.4s` |
| Constant fill | `a[i] = 0.0` | `DUP` + `ST1` (already handled by MemOp, but could be done in-JIT) |

**Tier 2 — Reductions:**

| Pattern | Example | NEON |
|---------|---------|------|
| Sum reduction | `sum += a[i]` | `FADD v.4s` + `FADDP` at end |
| Min/max reduction | `min = Math.min(min, a[i])` | `FMIN v.4s` + `FMINV` at end |
| Dot product | `sum += a[i] * b[i]` | `FMLA v.4s` + `FADDP` at end |

**Tier 3 — Masked/Conditional:**

| Pattern | Example | NEON |
|---------|---------|------|
| Conditional store | `if (a[i] > 0) b[i] = a[i]` | `FCMGT` + `BSL` |
| Clamping | `c[i] = Math.max(0, Math.min(1, a[i]))` | `FMAX` + `FMIN` |

### 5.4 Implementation Approach

Extend the existing MemOp infrastructure in `GlobOpt`:

1. `CollectMemOpInfo` currently only detects memset/memcopy patterns. Extend it to detect **vectorizable arithmetic** patterns.

2. Add a new `Loop::VectorizeCandidate` structure alongside `MemSetCandidate` and `MemCopyCandidate`:

```c++
struct VectorizeCandidate : public MemOpCandidate {
    Js::OpCode vectorOp;     // e.g., Simd128_Add_F4
    SymID srcA;              // source array A
    SymID srcB;              // source array B (or InvalidSymID for unary)
    IRType elementType;      // TyFloat32, TyInt32, etc.
    VectorizeCandidate() : MemOpCandidate(MemOpCandidate::VECTORIZE) {}
};
```

3. In `EmitMemop`, generate SIMD IR instead of helper calls:

```
// Before (scalar):
//   loop: LdElemI_A t1, a[i]
//         LdElemI_A t2, b[i]
//         Add_A     t3, t1, t2
//         StElemI_A c[i], t3
//         ...branch

// After (vectorized):
//   loop: Simd128_LdArr_F4 v1, a[i]    → LD1 {v0.4s}, [x0]
//         Simd128_LdArr_F4 v2, b[i]    → LD1 {v1.4s}, [x1]
//         Simd128_Add_F4   v3, v1, v2  → FADD v2.4s, v0.4s, v1.4s
//         Simd128_StArr_F4 c[i], v3    → ST1 {v2.4s}, [x2]
//         i += 4
//         ...branch
```

4. Generate a **scalar epilogue** for the remaining `length % 4` elements.

### 5.5 Scope Limitations

To keep complexity manageable, the initial auto-vectorizer should:

- **Only target TypedArray loops** (not generic JS arrays — their element types are not guaranteed)
- **Only vectorize innermost loops** (no nested loop vectorization)
- **Only handle stride-1 sequential access** (`a[i]`, not `a[i*2]` or `a[obj.x]`)
- **Require the loop bound to be provably TypedArray.length** (avoids out-of-bounds concerns)
- **Bail out if any call is found in the loop body** (calls can have arbitrary side effects)
- **Limit loop bodies to ≤ 3 simultaneously live 128-bit values** — the borrowed-register strategy provides Q27/Q28/Q29; patterns requiring more live vectors (e.g., 4-way unrolled loop bodies) are not candidates until full allocator integration
- **Respect `MinNeonSimdLoopCount` threshold** — loops with fewer iterations than the configured minimum (default 16) are not candidates, even if the pattern is vectorizable

### 5.6 Implementation Checklist — Phase 4

- [ ] Gate vectorization on `func->DoNeonVectorize()` (FullJit only, respects `-off:NeonVectorize`)
- [ ] Enforce `MinNeonSimdLoopCount` threshold in candidate collection (default 16)
- [ ] Extend `Loop::MemOpCandidate` with vectorization candidate type
- [ ] Add pattern matching for element-wise binary operations on typed arrays
- [ ] Add dependence analysis to reject loops with carried dependencies
- [ ] Add cost model (static iteration count + profiled loop count)
- [ ] Generate SIMD IR for vectorizable loops
- [ ] Generate scalar epilogue loop
- [ ] Add bailout for array detachment during vectorized execution
- [ ] Add `-trace:NeonVectorize` output showing accepted/rejected candidates with reasons
- [ ] Benchmark: tight arithmetic loops on Float32Array, Int32Array
- [ ] Test: edge cases (length=0, length=1, length=3, length=1000000)
- [ ] Test: `-MinNeonSimdLoopCount:4` (aggressive) and `-MinNeonSimdLoopCount:256` (conservative)

---

## 6. Phase 5: WASM SIMD on ARM64

**Effort:** Very High  
**Impact:** High — enables standard WebAssembly SIMD on Apple Silicon  
**Risk:** Medium  
**Dependencies:** Phase 3 (JIT NEON codegen)

### 6.1 Enable the Build

**File:** `lib/Common/CommonDefines.h`

```c++
// Current: ARM64 excluded
#if (defined(_M_IX86) || defined(_M_X64)) && !defined(DISABLE_JIT)
#define ASMJS_PLAT
#endif

// Proposed: ARM64 included
#if (defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM64)) && !defined(DISABLE_JIT)
#define ASMJS_PLAT
#endif
```

This unlocks `ENABLE_WASM`, `ENABLE_WASM_THREADS`, and `ENABLE_WASM_SIMD` on ARM64.

### 6.2 What Breaks

Enabling `ASMJS_PLAT` on ARM64 will expose:

1. **`_x86_SIMDValue`** — the `SimdUtils.h` struct uses `__m128` / `__m128i` / `__m128d` under `#if _M_IX86 || _M_AMD64`. Need an ARM64 equivalent using `float32x4_t` etc.
2. **`X86_TEMP_SIMD`** — global SIMD temp area in `ThreadContext.h`, x86-only.
3. **`LowerMDSharedSimd128.cpp`** — 3000+ lines of x86 SSE intrinsic lowering. All of it must be either gated with `#if _M_IX86 || _M_AMD64` or replaced with NEON equivalents for ARM64.
4. **`Simd128InitOpcodeMap`** — maps Chakra SIMD opcodes to x86 machine opcodes. Need ARM64 version.
5. **asm.js interpreter SIMD paths** — `InterpreterStackFrame::AsmJsInterpreterSimdJs` returns `__m128`, which doesn't exist on ARM64.
6. **Calling convention for SIMD return values** — x64 returns SIMD in XMM0; ARM64 returns in Q0/V0.

**Note on the 3-register constraint:** WASM SIMD programs can have arbitrary numbers of live v128 values — far more than the 3 borrowed scratch registers (Q27/Q28/Q29) can accommodate. For initial WASM SIMD bring-up, the lowerer can spill intermediate 128-bit values to 16-byte-aligned stack slots between NEON sequences, keeping the borrowed-register model. However, production-quality WASM SIMD performance will eventually require full Q-register integration into the linear scan allocator (expanding the `BitVector` and adding aliasing support). This is acceptable because WASM SIMD is a Phase 5 / P2 item — by the time it ships, Phase 3's borrowed-register approach will have been battle-tested and the allocator work can proceed with confidence.

### 6.3 ARM64 SIMDValue

```c++
#if defined(__aarch64__) || defined(_M_ARM64)
struct _arm64_SIMDValue {
    union {
        SIMD_DATA
        float32x4_t neon_f32;
        float64x2_t neon_f64;
        int32x4_t   neon_i32;
        int16x8_t   neon_i16;
        int8x16_t   neon_i8;
        uint8x16_t  neon_u8;
    };
    static _arm64_SIMDValue ToARM64SIMDValue(const SIMDValue& val) {
        _arm64_SIMDValue result;
        result.neon_f32 = vld1q_f32((const float*)&val);
        return result;
    }
    static SIMDValue ToSIMDValue(const _arm64_SIMDValue& val) {
        SIMDValue result;
        vst1q_f32((float*)&result, val.neon_f32);
        return result;
    }
};
typedef _arm64_SIMDValue ARM64SIMDValue;
typedef _arm64_SIMDValue PlatformSIMDValue;
#endif
```

### 6.4 Implementation Checklist — Phase 5

- [ ] Extend `ASMJS_PLAT` to include `_M_ARM64`
- [ ] Create `_arm64_SIMDValue` type alongside `_x86_SIMDValue`
- [ ] Gate all `__m128` / SSE code with `_M_IX86 || _M_AMD64`
- [ ] Create ARM64 `Simd128InitOpcodeMap` mapping SIMD opcodes to NEON MdOpcodes
- [ ] Implement ARM64 `Simd128Instruction` / `Simd128LowerUnMappedInstruction`
- [ ] Fix asm.js SIMD interpreter entry/return for ARM64 calling convention
- [ ] Fix `X86_TEMP_SIMD` → platform-generic `TEMP_SIMD`
- [ ] Run `test/wasm.simd/` test suite on ARM64
- [ ] Validate with real WASM SIMD binaries (e.g., compiled with `clang -msimd128`)

---

## 7. Architecture Reference

### ARM64 NEON Quick Reference

| Instruction | Description | Lanes |
|-------------|-------------|-------|
| `LD1 {Vt.4S}, [Xn]` | Load 4×float32 | 4 |
| `LD1 {Vt.2D}, [Xn]` | Load 2×float64 | 2 |
| `ST1 {Vt.4S}, [Xn]` | Store 4×float32 | 4 |
| `DUP Vd.4S, Wn` | Broadcast GPR to all lanes | 4 |
| `FADD Vd.4S, Vn.4S, Vm.4S` | Vector float add | 4 |
| `FSUB Vd.4S, Vn.4S, Vm.4S` | Vector float sub | 4 |
| `FMUL Vd.4S, Vn.4S, Vm.4S` | Vector float mul | 4 |
| `FDIV Vd.4S, Vn.4S, Vm.4S` | Vector float div | 4 |
| `FMIN Vd.4S, Vn.4S, Vm.4S` | Vector float min | 4 |
| `FMAX Vd.4S, Vn.4S, Vm.4S` | Vector float max | 4 |
| `FABS Vd.4S, Vn.4S` | Vector float abs | 4 |
| `FNEG Vd.4S, Vn.4S` | Vector float neg | 4 |
| `FSQRT Vd.4S, Vn.4S` | Vector float sqrt | 4 |
| `SCVTF Vd.4S, Vn.4S` | Int32→Float32 convert | 4 |
| `FCVTZS Vd.4S, Vn.4S` | Float32→Int32 truncate | 4 |
| `ADD Vd.4S, Vn.4S, Vm.4S` | Vector int add | 4 |
| `SUB Vd.4S, Vn.4S, Vm.4S` | Vector int sub | 4 |
| `MUL Vd.4S, Vn.4S, Vm.4S` | Vector int mul | 4 |
| `AND Vd.16B, Vn.16B, Vm.16B` | Bitwise AND | 16 |
| `ORR Vd.16B, Vn.16B, Vm.16B` | Bitwise OR | 16 |
| `EOR Vd.16B, Vn.16B, Vm.16B` | Bitwise XOR | 16 |
| `BSL Vd.16B, Vn.16B, Vm.16B` | Bit select | 16 |
| `CMEQ Vd.4S, Vn.4S, Vm.4S` | Compare equal (int) | 4 |
| `CMGT Vd.4S, Vn.4S, Vm.4S` | Compare greater (int) | 4 |
| `FCMEQ Vd.4S, Vn.4S, Vm.4S` | Compare equal (float) | 4 |
| `FCMGT Vd.4S, Vn.4S, Vm.4S` | Compare greater (float) | 4 |
| `ADDV Sd, Vn.4S` | Horizontal add all lanes | 1 (result) |
| `FADDP Vd.4S, Vn.4S, Vm.4S` | Pairwise float add | 4 |
| `UMAXV Bd, Vn.16B` | Horizontal unsigned max | 1 (result) |
| `TBL Vd.16B, {Vn.16B}, Vm.16B` | Table lookup (shuffle) | 16 |
| `REV64 Vd.4S, Vn.4S` | Reverse in 64-bit halves | 4 |
| `EXT Vd.16B, Vn.16B, Vm.16B, #imm` | Extract/concatenate | 16 |

### Apple Silicon NEON Performance Characteristics

| Feature | Apple M1/M2/M3/M4 |
|---------|-------------------|
| NEON register count | 32 × 128-bit |
| Peak NEON throughput | 4 × 128-bit ops/cycle (M1+) |
| LD1 throughput | 2 × 128-bit loads/cycle |
| ST1 throughput | 1–2 × 128-bit stores/cycle |
| FADD latency | 2 cycles |
| FMUL latency | 3 cycles |
| FDIV latency | 7–10 cycles |
| FSQRT latency | 7–10 cycles |
| Alignment requirement | None (unaligned OK, no penalty on Apple Silicon) |

---

## 8. Testing Strategy

### 8.1 Correctness Tests

| Test Category | Description |
|---------------|-------------|
| TypedArray fill | Fill with 0, 1, -1, NaN, Infinity, -0, MAX_VALUE for all 8 types |
| TypedArray indexOf | Search for present, absent, NaN, -0 values |
| TypedArray reverse | Even/odd length, length=0, length=1 |
| Min/max | Arrays with NaN, -0/+0, all-same, sorted, reverse-sorted |
| SIMD operations | All Simd*Operation functions against scalar reference |
| Edge cases | length=0, 1, 3, 4, 5, 15, 16, 17, 1000000 |
| Alignment | Buffers at offset 0, 4, 8, 12 from 16-byte boundary |

### 8.2 Benchmark Suite

Create `tests/neon_benchmarks/`:

```javascript
// fill_bench.js
const N = 1_000_000;
const a = new Float32Array(N);
const start = Date.now();
for (let iter = 0; iter < 1000; iter++) {
    a.fill(3.14);
}
print(`Float32Array.fill: ${Date.now() - start}ms`);

// add_bench.js
const a = new Float32Array(N);
const b = new Float32Array(N);
const c = new Float32Array(N);
for (let i = 0; i < N; i++) { a[i] = i; b[i] = i * 0.5; }
const start = Date.now();
for (let iter = 0; iter < 1000; iter++) {
    for (let i = 0; i < N; i++) c[i] = a[i] + b[i];
}
print(`Float32Array add loop: ${Date.now() - start}ms`);

// indexof_bench.js
const a = new Int32Array(N);
for (let i = 0; i < N; i++) a[i] = i;
const start = Date.now();
for (let iter = 0; iter < 10000; iter++) {
    a.indexOf(N - 1);  // worst case: last element
}
print(`Int32Array.indexOf: ${Date.now() - start}ms`);
```

### 8.3 Regression Prevention

- Run existing test suite after every phase to catch regressions
- Run `test/Array/`, `test/typedarray/`, `test/AsmJs/` suites specifically
- Compare output of NEON paths vs scalar paths on randomized inputs

---

## 9. Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| NaN/±0 semantics differ between NEON and JS spec | Medium | Extensive edge-case tests; NEON's `FMIN`/`FMAX` do propagate NaN but may differ on ±0 ordering — verify and add fixup if needed |
| Detached ArrayBuffer during NEON operation | High | Check detach before NEON path; NEON operates on raw pointer so must not be called if buffer could detach mid-operation |
| Borrowed NEON registers reduce FP pressure headroom | Low | 27 D-registers (D0–D26) remain allocatable — JS workloads rarely need more than a handful of live FP values; stress-test with FP-heavy benchmarks to confirm |
| 3-register limit constrains WASM SIMD | Medium | Sufficient for JS-level NEON and initial WASM SIMD (with stack spills); full allocator integration deferred to when WASM SIMD matures — see Phase 5 note |
| Unaligned memory access | Low | Apple Silicon has no penalty for unaligned NEON access; other ARM64 targets may differ |
| Compiler auto-vectorizing our scalar code anyway | Low | Check with `-O2` disassembly; if the compiler is already vectorizing, our manual NEON intrinsics just make it explicit and guaranteed |
| Code size increase | Low | NEON paths are gated with `#if`; binary size increase is modest |
| Apple Silicon specific behavior | Low | NEON is standard ARM64; all instructions used are in the base ARMv8-A spec |

---

## 10. Priority Matrix

| Phase | Effort | Impact | Risk | Priority | Estimated Time |
|-------|--------|--------|------|----------|---------------|
| **1: C++ NEON Intrinsics** | Medium | High | Low | **🔴 P0** | 2–3 weeks |
| **2: JIT Helper Acceleration** | Medium | High | Low | **🔴 P0** | 1–2 weeks |
| **3: JIT NEON Codegen** | Medium | Very High | Low | **🟡 P1** | 2–3 weeks |
| **4: Auto-Vectorization** | Very High | Transformative | High | **🟢 P2** | 8–12 weeks |
| **5: WASM SIMD** | Very High | High | Medium–High | **🟢 P2** | 6–8 weeks |

> **Phase 3 revision note:** Effort reduced from High→Medium and risk from Medium→Low because the borrowed-register strategy (Q27/Q28/Q29 as `RA_DONTALLOCATE`) eliminates the register allocator rework that was the primary engineering and regression risk. Phase 5 risk increased to Medium–High because WASM SIMD will eventually require the deferred full allocator integration.

### Recommended Execution Order

1. **Phase 1** first — immediate wins, no JIT risk, benefits every execution mode
2. **Phase 2** alongside Phase 1 — small delta on top of Phase 1's NEON helpers
3. **Phase 3** as the main engineering push — unlocks everything else
4. **Phase 4 and 5** can proceed in parallel once Phase 3 is solid

### Quick Wins (< 1 day each)

These individual changes from Phase 1 can be landed independently:

1. NEON `SimdFloat32x4Operation::OpAdd/Sub/Mul` — ~10 minutes each, 4× speedup
2. NEON `TypedArray::DirectSetItemAtRange` for `Float32Array` — ~1 hour
3. NEON `CopyValueToSegmentBuferNoCheck<int32>` for non-zero fill — ~30 minutes

---

## Appendix A: File Index

All files that need modification, by phase:

### Phase 1
| File | Change |
|------|--------|
| `lib/Runtime/Language/NeonAccel.h` | **NEW** — NEON utility header |
| `lib/Runtime/Language/SimdFloat32x4Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdFloat64x2Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdInt32x4Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdInt16x8Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdInt8x16Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdUint32x4Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdUint16x8Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdUint8x16Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdBool32x4Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdBool16x8Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdBool8x16Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Language/SimdInt64x2Operation.cpp` | Replace ARM scalar with NEON intrinsics |
| `lib/Runtime/Library/TypedArray.h` | Add NEON fill paths in `DirectSetItemAtRange` |
| `lib/Runtime/Library/JavascriptArray.cpp` | NEON `CopyValueToSegmentBuferNoCheck`, NEON `HeadSegmentIndexOfHelper` |
| `lib/Runtime/Library/TypedArray.cpp` | NEON `FindMinOrMax` |

### Phase 2
| File | Change |
|------|--------|
| `lib/Runtime/Language/JavascriptOperators.cpp` | NEON dispatch in `OP_Memset`, `OP_Memcopy` |

### Phase 3
| File | Change |
|------|--------|
| `lib/Common/ConfigFlagsList.h` | Add `PHASE(NeonSimd)`, `PHASE(NeonVectorize)`, `MinNeonSimdLoopCount` flag |
| `lib/Backend/Func.h` | Add `DoNeonSimd()`, `DoNeonVectorize()` helpers (FullJit gate) |
| `lib/Backend/arm64/MdOpCodes.h` | Add ~30 NEON vector opcodes |
| `lib/Backend/arm64/EncoderMD.cpp` | Encode new NEON opcodes |
| `lib/Backend/arm64/RegList.h` | Mark D27–D29 as `RA_DONTALLOCATE` (3-line change) |
| `lib/Backend/arm64/NeonRegs.h` | **NEW** — scratch register definitions and usage convention |
| `lib/Backend/arm64/LowerMD.cpp` | Wire up SIMD lowering dispatch |
| `lib/Backend/arm64/LowerMDSimd128.cpp` | **NEW** — ARM64 SIMD instruction lowering (uses fixed Q27/Q28/Q29) |
| `lib/Backend/arm64/LegalizeMD.cpp` | Legalize NEON instructions |

### Phase 4
| File | Change |
|------|--------|
| `lib/Backend/FlowGraph.h` | Add `VectorizeCandidate` structure |
| `lib/Backend/GlobOpt.cpp` | Extend `CollectMemOpInfo` for vectorization patterns |
| `lib/Backend/GlobOpt.h` | Add vectorization analysis methods |
| `lib/Backend/BackwardPass.cpp` | Handle vectorized loop dead store elimination |

### Phase 5
| File | Change |
|------|--------|
| `lib/Common/CommonDefines.h` | Extend `ASMJS_PLAT` to ARM64 |
| `lib/Runtime/Language/SimdUtils.h` | Add `_arm64_SIMDValue` type |
| `lib/Runtime/Base/ThreadContext.h` | Platform-generic SIMD temp area |
| `lib/Runtime/Language/InterpreterStackFrame.cpp` | ARM64 SIMD interpreter entry |
| `lib/Runtime/Language/InterpreterStackFrame.h` | ARM64 SIMD return type |
| `lib/WasmReader/WasmBinaryOpcodesSimd.h` | Verify ARM64 compatibility |
| `lib/Backend/LowerMDSharedSimd128.cpp` | Gate x86 code, add ARM64 dispatch |

---

## Appendix B: Estimated Performance Gains

| Operation | Current (scalar) | With NEON | Speedup |
|-----------|-----------------|-----------|---------|
| `Float32Array.fill(v)` 1M elements | ~1.0ms | ~0.06ms | **~16×** |
| `Float64Array.fill(v)` 1M elements | ~2.0ms | ~0.25ms | **~8×** |
| `Int8Array.fill(v)` 1M elements | ~0.5ms | ~0.03ms | **~16×** |
| `Float32Array` add loop 1M elements | ~4.0ms | ~1.0ms | **~4×** |
| `Int32Array.indexOf` 1M elements (worst) | ~2.0ms | ~0.5ms | **~4×** |
| `Int8Array.indexOf` 1M elements (worst) | ~2.0ms | ~0.12ms | **~16×** |
| `Float32Array` min/max 1M elements | ~3.0ms | ~0.75ms | **~4×** |
| `Float32Array.reverse()` 1M elements | ~2.0ms | ~0.5ms | **~4×** |
| `SimdFloat32x4Operation::OpAdd` | ~4 scalar ops | 1 NEON op | **~4×** |

*Estimates based on Apple M1 throughput characteristics. Actual gains will depend on memory bandwidth saturation, cache effects, and loop overhead.*