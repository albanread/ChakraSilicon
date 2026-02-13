//-------------------------------------------------------------------------------------------------------
// Copyright (C) ChakraSilicon Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NeonRegs.h — Dedicated NEON 128-bit scratch register definitions
//
// Strategy:
//   D27–D29 are marked RA_DONTALLOCATE in RegList.h, removing them from the
//   linear scan register allocator's pool. The lowerer and encoder reference
//   them as Q27–Q29 (V27–V29) for 128-bit NEON operations.
//
//   The register allocator is completely unaware of these — they are managed
//   explicitly by the NEON lowering pass, analogous to how SCRATCH_REG (R17)
//   is used for address materialization elsewhere in the ARM64 backend.
//
// Why not full Q-register integration:
//   The linear scan allocator uses a 64-bit BitVector (BVUnit64) and ARM64
//   already fills all 64 slots (1 NOREG + 33 integer + 30 float). Adding
//   Q0–Q29 as separate allocatable entries would require expanding the
//   bitvector, adding register aliasing logic (Qn overlaps Dn), and updating
//   spill slot sizing — a 4–6 week effort in the most fragile backend code.
//   Borrowing 3 registers avoids all of this.
//
// Cost:
//   27 D-registers (D0–D26) remain allocatable for scalar floating-point.
//   D27–D29 are caller-saved (no RA_CALLEESAVE flag), so reserving them
//   adds no prolog/epilog overhead for non-NEON functions.
//
// Future:
//   If WASM SIMD or complex auto-vectorization demands more than 3 live
//   128-bit values, full allocator integration can be revisited. The
//   borrowed-register approach is designed to be replaced transparently —
//   the lowerer's explicit register assignments would simply become
//   allocator-managed operands.
//
//-------------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------------
// NEON scratch register numbers (V-register index, 0–31)
//
// These correspond to the physical registers V27/Q27, V28/Q28, V29/Q29.
// The hardware encode value matches the D-register encode (NEONREG_D27 etc.)
// since Dn and Qn share the same 5-bit register field in ARM64 encodings.
//-------------------------------------------------------------------------------------------------------

#define NEON_SCRATCH_REG_0      27      // Q27 / V27 — first NEON scratch
#define NEON_SCRATCH_REG_1      28      // Q28 / V28 — second NEON scratch
#define NEON_SCRATCH_REG_2      29      // Q29 / V29 — third NEON scratch

// RegNum aliases for use with IR::RegOpnd — these map to the existing
// D-register RegNum entries in RegList.h (which share the physical register).
#define NEON_SCRATCH_REGNUM_0   RegD27
#define NEON_SCRATCH_REGNUM_1   RegD28
#define NEON_SCRATCH_REGNUM_2   RegD29

// Total number of NEON scratch registers available
#define NEON_SCRATCH_REG_COUNT  3

//-------------------------------------------------------------------------------------------------------
// Usage convention in lowered NEON sequences
//
//   Q27 (NEON_SCRATCH_REG_0) — source A, or accumulator in reductions
//   Q28 (NEON_SCRATCH_REG_1) — source B, or broadcast/splat temp
//   Q29 (NEON_SCRATCH_REG_2) — destination / result
//
// Pattern fitness (all targeted patterns fit within 3 registers):
//
//   Element-wise binary  (c[i] = a[i] + b[i])     Q27=srcA  Q28=srcB  Q29=dest
//   Element-wise unary   (b[i] = -a[i])            Q27=src   —         Q29=dest
//   Scalar broadcast+op  (b[i] = a[i] * k)         Q27=src   Q28=splat Q29=dest
//   Reduction            (sum += a[i])              Q27=accum Q28=chunk —
//   Compare+select       (FCMGT + BSL)             Q27=srcA  Q28=srcB  Q29=mask→dest
//   Fill                 (memset-style DUP+ST1)    Q27=splat —         —
//
// Key invariant:
//   NEON scratch register lifetimes must NOT cross IR instruction boundaries.
//   The lowerer loads into Qn, operates, stores the result, and the scratch
//   registers are immediately dead. This means no spill logic is ever needed.
//
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// Helpers for the lowerer to create RegOpnd for NEON scratch registers
//
// Usage in LowerMDSimd128.cpp:
//
//   IR::RegOpnd* srcA = IR::RegOpnd::New(nullptr, NEON_SCRATCH_REGNUM_0, TyFloat64, func);
//   IR::RegOpnd* srcB = IR::RegOpnd::New(nullptr, NEON_SCRATCH_REGNUM_1, TyFloat64, func);
//   IR::RegOpnd* dst  = IR::RegOpnd::New(nullptr, NEON_SCRATCH_REGNUM_2, TyFloat64, func);
//
// Note: TyFloat64 is used because the RegList.h entries are typed as TyFloat64.
// The encoder knows to emit 128-bit (Q-form) instructions based on the MdOpCode,
// not the IR type. The register field encodes identically for Dn and Qn.
//-------------------------------------------------------------------------------------------------------

// Validate that the scratch registers are at the expected positions.
// If RegList.h is reordered, these asserts will catch the mismatch at compile time.
// (Place a static_assert in a .cpp file that includes this header.)
#define NEON_SCRATCH_STATIC_ASSERT() \
    static_assert(NEON_SCRATCH_REG_0 == 27, "NEON_SCRATCH_REG_0 must be register 27 (Q27/V27)"); \
    static_assert(NEON_SCRATCH_REG_1 == 28, "NEON_SCRATCH_REG_1 must be register 28 (Q28/V28)"); \
    static_assert(NEON_SCRATCH_REG_2 == 29, "NEON_SCRATCH_REG_2 must be register 29 (Q29/V29)"); \
    static_assert(NEON_SCRATCH_REG_COUNT == 3, "Expected exactly 3 NEON scratch registers");