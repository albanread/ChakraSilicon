# WASM ARM64 Port — Feasibility Analysis & Implementation Plan

**Date:** February 2025  
**Status:** Analysis Complete — Ready to Implement  
**Scope:** Porting WebAssembly support from x86/x64 backend to ARM64 (Apple Silicon)

---

## Executive Summary

The ChakraCore WASM implementation is fully functional on x86/x64 but **completely disabled on ARM64**. A single preprocessor gate (`ASMJS_PLAT`) excludes ARM64, leaving 17 stub functions in the ARM64 backend that `Assert(UNREACHED)`.

The existing ARM64 JS JIT — which is fully working (SimpleJit + FullJit, ~86% test pass rate) — provides **~80% of the infrastructure** needed for WASM. The core work is approximately 12 function implementations, most of which are thin wrappers around call lowering, bounds checking, and memory operations that the ARM64 JIT already supports.

**WASM SIMD** (mapping x86 SSE → ARM64 NEON) is a separate, larger effort (~3,300 lines of x86-specific code) but is deferrable. The NEON encoder is already complete (5,022 lines, 400+ operations).

---

## Table of Contents

1. [Why WASM Is Disabled on ARM64](#1-why-wasm-is-disabled-on-arm64)
2. [Architecture of WASM in ChakraCore](#2-architecture-of-wasm-in-chakracore)
3. [What the ARM64 JS JIT Already Provides](#3-what-the-arm64-js-jit-already-provides)
4. [The 17 Stub Functions](#4-the-17-stub-functions)
5. [Per-Function Implementation Analysis](#5-per-function-implementation-analysis)
6. [New MdOpCodes & Encoder Work](#6-new-mdopcodes--encoder-work)
7. [Memory Model Considerations](#7-memory-model-considerations)
8. [WASM SIMD — Deferred Phase](#8-wasm-simd--deferred-phase)
9. [Enablement Steps](#9-enablement-steps)
10. [Risk Assessment](#10-risk-assessment)
11. [File Inventory](#11-file-inventory)
12. [Estimated Effort](#12-estimated-effort)

---

## 1. Why WASM Is Disabled on ARM64

A single macro chain in `lib/Common/CommonDefines.h` gates everything:

```c
// CommonDefines.h L697-706
#if (defined(_M_IX86) || defined(_M_X64)) && !defined(DISABLE_JIT)
#define ASMJS_PLAT
#endif

#if defined(ASMJS_PLAT)
#define ENABLE_WASM
#define ENABLE_WASM_THREADS
#define ENABLE_WASM_SIMD
#endif
```

`_M_ARM64` is not included in the `ASMJS_PLAT` condition, so:
- `ENABLE_WASM` — **undefined** on ARM64
- `ENABLE_WASM_THREADS` — **undefined** on ARM64
- `ENABLE_WASM_SIMD` — **undefined** on ARM64

This cascades through the entire codebase. All WASM-guarded code compiles out. The ARM64 backend's `LowerMD.h` has inline stubs that crash if somehow reached:

```c
// lib/Backend/arm64/LowerMD.h L166-172
IR::Instr * LowerAsmJsCallI(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
IR::Instr * LowerAsmJsCallE(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
IR::Instr * LowerWasmArrayBoundsCheck(IR::Instr * instr, IR::Opnd *addrOpnd) { Assert(UNREACHED); return nullptr; }
void        LowerAtomicStore(IR::Opnd * dst, IR::Opnd * src1, IR::Instr * insertBeforeInstr) { Assert(UNREACHED); }
void        LowerAtomicLoad(IR::Opnd* dst, IR::Opnd* src1, IR::Instr* insertBeforeInstr) { Assert(UNREACHED); }
IR::Instr * LowerAsmJsStElemHelper(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
IR::Instr * LowerAsmJsLdElemHelper(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
```

Additionally, `ENABLE_FAST_ARRAYBUFFER` (guard-page based bounds elision) is Windows x64 only:

```c
// CommonDefines.h L302-304
#if defined(_WIN32) && defined(TARGET_64) && !defined(_M_ARM64)
#define ENABLE_FAST_ARRAYBUFFER 1
#endif
```

This means ARM64 WASM will always need explicit bounds checks — which is actually simpler.

---

## 2. Architecture of WASM in ChakraCore

WASM flows through the same JIT pipeline as JavaScript, with WASM-specific stages at the front end and WASM-specific lowering at the back end:

```
┌──────────────────────────────────────────────────────────┐
│  WASM Binary (.wasm)                                     │
│         │                                                │
│         ▼                                                │
│  WasmBinaryReader  (lib/WasmReader/, ~6,400 lines)       │  ← Architecture-independent
│         │                                                │
│         ▼                                                │
│  WasmByteCodeGenerator → AsmJs Bytecodes                 │  ← Architecture-independent
│         │                                                │
│         ▼                                                │
│  IRBuilderAsmJs (lib/Backend/IRBuilderAsmJs.cpp)         │  ← Architecture-independent
│         │                                                │
│         ▼                                                │
│  GlobOpt → BackwardPass → Lowerer (shared Lower.cpp)     │  ← Architecture-independent
│         │                                                │
│         ▼                                                │
│  LowererMD (arm64/LowerMD.cpp)  ◄── THIS IS THE GAP     │  ← Architecture-specific
│         │                                                │
│         ▼                                                │
│  Encoder (arm64/EncoderMD.cpp) → Native ARM64 code       │  ← Architecture-specific
└──────────────────────────────────────────────────────────┘
```

The key insight: **the gap is only in the bottom two layers**. Everything from WASM binary parsing through IR building and optimization is completely architecture-independent and will "just work" once the macros are enabled.

### Shared Lowerer Dispatch (already handles WASM)

`lib/Backend/Lower.cpp` (29,394 lines) already dispatches WASM IR opcodes to the machine-dependent lowerer. These call sites exist and will compile for ARM64 once `ENABLE_WASM` is defined:

| IR Opcode | Lower.cpp Handler | Calls Into |
|-----------|-------------------|------------|
| `AsmJsCallI` | `LowerRange()` L878 | `m_lowererMD.LowerAsmJsCallI()` |
| `AsmJsCallE` | `LowerRange()` L882 | `m_lowererMD.LowerAsmJsCallE()` |
| `LdArrViewElemWasm` | `LowerLdArrViewElemWasm()` L9592 | `LowerWasmArrayBoundsCheck()` → `m_lowererMD.LowerWasmArrayBoundsCheck()` |
| `StArrViewElem` (wasm) | `LowerStArrViewElem()` L9856 | `LowerWasmArrayBoundsCheck()` → `m_lowererMD.LowerWasmArrayBoundsCheck()` |
| `StAtomicWasm` | `LowerStAtomicsWasm()` L9780 | `m_lowererMD.LowerAtomicStore()` |
| `LdAtomicWasm` | `LowerLdAtomicsWasm()` L9806 | `m_lowererMD.LowerAtomicLoad()` |
| `CheckWasmSignature` | `LowerCheckWasmSignature()` L8548 | Helper call (arch-independent) |
| `LdWasmFunc` | `LowerLdWasmFunc()` | Helper call (arch-independent) |
| `GrowWasmMemory` | `LowerGrowWasmMemory()` | Helper call (arch-independent) |
| `Copysign_A` | `LowerRange()` L3018 | `m_lowererMD.GenerateCopysign()` |

---

## 3. What the ARM64 JS JIT Already Provides

The ARM64 JIT is working and battle-tested. Here is what can be directly reused for WASM:

### Fully Working Infrastructure

| Component | Lines | Key Files | WASM Reuse |
|-----------|-------|-----------|------------|
| ARM64 Instruction Encoder | 4,610 | `arm64/ARM64Encoder.h` | Direct — includes `LDAR`, `STLR`, `DMB` for atomics |
| NEON/SIMD Encoder | 5,022 | `arm64/ARM64NeonEncoder.h` | Direct — 400+ NEON ops, ready for WASM SIMD |
| Machine Instruction Encoding | 1,673 | `arm64/EncoderMD.cpp` | Direct — needs ~5 new opcode cases for atomics |
| Machine Opcodes | 134 | `arm64/MdOpCodes.h` | Needs ~5 new entries |
| Register Allocator | — | `arm64/LinearScanMD.cpp` | Direct — no changes needed |
| Lowerer (7,488 lines) | 7,488 | `arm64/LowerMD.cpp` | Core call/branch/memory lowering reused |
| Instruction Legalization | — | `arm64/LegalizeMD.cpp` | Direct — no changes needed |
| Prolog/Epilog | — | `arm64/LowerMD.cpp` L1222-1655 | Template for AsmJs epilog |
| DWARF `.eh_frame` | — | `EhFrame.cpp`, `PrologEncoder.cpp` | Direct — already corrected for ARM64 macOS |
| W^X Memory Protection | — | `CustomHeap.cpp`, `VirtualAllocWrapper.cpp` | Direct — `MAP_JIT` + `pthread_jit_write_protect_np` |
| DarwinPCS Calling Convention | — | `LowerMD.cpp`, `Arguments.h` | Direct — variadic ABI fix applies to WASM calls too |
| Exception Handling | — | `JavascriptExceptionOperators.cpp` | Direct — `setjmp`/`longjmp` approach |

### Specific ARM64 Instructions Already Available in the Encoder

These are defined in `ARM64Encoder.h` but not all are wired into `MdOpCodes.h`/`EncoderMD.cpp` yet:

| Instruction | Purpose | Encoder Function | In MdOpCodes? |
|-------------|---------|------------------|---------------|
| `LDAR` / `LDARB` / `LDARH` | Load-acquire (atomic read) | `EmitLdar`, `EmitLdar64`, `EmitLdarb`, `EmitLdarh` | ❌ Needs adding |
| `STLR` / `STLRB` / `STLRH` | Store-release (atomic write) | `EmitStlr`, `EmitStlr64`, `EmitStlrb`, `EmitStlrh` | ❌ Needs adding |
| `DMB ISH` | Data memory barrier | `EmitDmb` | ❌ Needs adding |
| `FMOV_GEN` | Bitwise move GP↔FP | Already in MdOpCodes | ✅ Available |
| `FCVT` | Float32↔Float64 conversion | Already in MdOpCodes | ✅ Available |
| `FCVTZ` | Float to int (truncate) | Already in MdOpCodes | ✅ Available |
| `SBFX` | Signed bitfield extract (sign extend) | Already in MdOpCodes | ✅ Available |
| `AND`/`ORR`/`EOR` | Bitwise (for copysign) | Already in MdOpCodes | ✅ Available |

---

## 4. The 17 Stub Functions

All `Assert(UNREACHED)` stubs in `arm64/LowerMD.h` that need real implementations:

### WASM Core (7 functions — required)

| # | Function | Purpose | Stub Location |
|---|----------|---------|---------------|
| 1 | `LowerAsmJsCallI` | Internal WASM function call | `LowerMD.h` L166 |
| 2 | `LowerAsmJsCallE` | External WASM function call | `LowerMD.h` L167 |
| 3 | `LowerWasmArrayBoundsCheck` | Linear memory bounds check | `LowerMD.h` L168 |
| 4 | `LowerAtomicStore` | WASM threads: atomic store | `LowerMD.h` L169 |
| 5 | `LowerAtomicLoad` | WASM threads: atomic load | `LowerMD.h` L170 |
| 6 | `LowerAsmJsStElemHelper` | Bounds-checked memory store | `LowerMD.h` L171 |
| 7 | `LowerAsmJsLdElemHelper` | Bounds-checked memory load | `LowerMD.h` L172 |

### WASM Type Operations (6 functions — required)

| # | Function | Purpose | Stub Location |
|---|----------|---------|---------------|
| 8 | `EmitFloat32ToFloat64` | Widen f32→f64 | `LowerMD.h` L187 |
| 9 | `EmitInt64toFloat` | Convert i64→f32/f64 | `LowerMD.h` L188 |
| 10 | `EmitSignExtend` | Sign-extend i8/i16/i32→i64 | `LowerMD.h` L193 |
| 11 | `EmitReinterpretPrimitive` | Bitcast int↔float | `LowerMD.h` L194 |
| 12 | `EmitReinterpretFloatToInt` | Wrapper for above | (inline in header) |
| 13 | `EmitReinterpretIntToFloat` | Wrapper for above | (inline in header) |

### WASM Frame/Calling (2 functions — required)

| # | Function | Purpose | Stub Location |
|---|----------|---------|---------------|
| 14 | `LowerExitInstrAsmJs` | AsmJs/WASM epilog | `LowerMD.h` L202 |
| 15 | `GenerateCopysign` | WASM `f32.copysign`/`f64.copysign` | Not declared in ARM64 header |

### Possibly Needed (2 functions — conditional)

| # | Function | Purpose | Notes |
|---|----------|---------|-------|
| 16 | `GenerateTruncWithCheck` | WASM `i32.trunc_f*` | May already be implemented (needs verification) |
| 17 | `HelperCallForAsmMathBuiltin` | Fallback for `trunc`/`nearest` | May need WASM-specific paths |

---

## 5. Per-Function Implementation Analysis

### 5.1 `LowerAsmJsCallI` — Effort: **Trivial**

**AMD64 reference** (`amd64/LowererMDArch.cpp` L1093-1101):
```c
IR::Instr *
LowererMDArch::LowerAsmJsCallI(IR::Instr * callInstr)
{
    int32 argCount = this->LowerCallArgs(callInstr, Js::CallFlags_Value, 0);
    IR::Instr* ret = this->LowerCall(callInstr, argCount);
    return ret;
}
```

**ARM64 implementation:** Direct port. `LowerCallArgs` and `LowerCall` already exist in `arm64/LowerMD.cpp` (L661-387). The DarwinPCS calling convention fixes are already applied. No extra params (`extraParams = 0`), no CallInfo needed.

**Estimated lines:** ~10

---

### 5.2 `LowerAsmJsCallE` — Effort: **Trivial**

**AMD64 reference** (`amd64/LowererMDArch.cpp` L1082-1091):
```c
IR::Instr *
LowererMDArch::LowerAsmJsCallE(IR::Instr *callInstr)
{
    IR::IntConstOpnd *callInfo = nullptr;
    int32 argCount = this->LowerCallArgs(callInstr, Js::CallFlags_Value, 1, &callInfo);
    IR::Instr* ret = this->LowerCall(callInstr, argCount);
    return ret;
}
```

**ARM64 implementation:** Same as `LowerAsmJsCallI` but with `extraParams = 1` for the function object. Direct port.

**Estimated lines:** ~10

---

### 5.3 `LowerWasmArrayBoundsCheck` — Effort: **Moderate**

**AMD64 reference** (`amd64/LowererMDArch.cpp` L1103-1150):

The function:
1. Validates the index operand is 32-bit integral
2. Checks for fast virtual buffer (skip bounds check) — **not applicable on ARM64 macOS**
3. Computes `index + offset + access_size`
4. Compares against array buffer length
5. Generates `WASMERR_ArrayIndexOutOfRange` throw on overflow

**ARM64 implementation:** Nearly direct port. The comparison and branching use architecture-independent helpers (`InsertCompareBranch`, `InsertAdd`, `InsertBranch`, `Lowerer::InsertLabel`). The only x86-ism is `UsesWAsmJsFastVirtualBuffer()` which returns false on ARM64 anyway.

Key difference: On ARM64, `ADDS` sets flags for overflow detection (vs x86 `ADD` + `JO`). The existing ARM64 `InsertAdd(true, ...)` with `setConditionCode=true` already handles this.

**Estimated lines:** ~50

---

### 5.4 `LowerAtomicStore` — Effort: **Moderate** (different approach from x86)

**AMD64 reference** (`amd64/LowererMDArch.cpp` L1148-1159):
```c
// x86 uses XCHG which is inherently locked
IR::RegOpnd* tmpSrc = IR::RegOpnd::New(dst->GetType(), m_func);
Lowerer::InsertMove(tmpSrc, src1, insertBeforeInstr);
IR::Instr* xchgInstr = IR::Instr::New(Js::OpCode::XCHG, tmpSrc, tmpSrc, dst, m_func);
insertBeforeInstr->InsertBefore(xchgInstr);
```

**ARM64 implementation:** ARM64 does not have `XCHG`. Instead:
1. `DMB ISH` — full barrier before store
2. `STLR` — store-release (provides release semantics)

This requires new `STLR` and `DMB` opcodes in `MdOpCodes.h` and encoder wiring. The encoder functions (`EmitStlr`, `EmitStlr64`, `EmitDmb`) already exist in `ARM64Encoder.h`.

```
// Pseudo-implementation:
DMB ISH              ; full memory barrier
STLR src, [addr]     ; store-release
```

**Estimated lines:** ~20

---

### 5.5 `LowerAtomicLoad` — Effort: **Moderate** (different approach from x86)

**AMD64 reference** (`amd64/LowererMDArch.cpp` L1161-1186):
```c
// x86 uses MOV (already atomic for aligned loads) + LOCK OR [rsp], 0 (memory barrier)
IR::Instr* newMove = Lowerer::InsertMove(dst, src1, insertBeforeInstr);
IR::Instr* memoryBarrier = IR::Instr::New(Js::OpCode::LOCKOR, stackTop, stackTop, zero, m_func);
newMove->InsertBefore(memoryBarrier);
```

**ARM64 implementation:** Use `LDAR` (load-acquire):
1. `LDAR dst, [addr]` — load-acquire (provides acquire semantics)
2. No extra barrier needed — `LDAR` is self-fencing on ARM64

The encoder already has `EmitLdar` / `EmitLdar64` / `EmitLdarb` / `EmitLdarh`.

```
// Pseudo-implementation:
LDAR dst, [addr]     ; load-acquire (implicit acquire barrier)
```

**Estimated lines:** ~15

---

### 5.6 `LowerAsmJsLdElemHelper` — Effort: **Moderate**

**AMD64 reference** (`amd64/LowererMDArch.cpp` L1188-1295):

Generates bounds-check-and-load pattern:
```
CMP index, arraySize
JGE $helper
JMP $load
$helper:
  MOV dst, 0 / NaN       ; out-of-bounds default value
  JMP $done
$load:
  MOV dst, [buffer + index]
$done:
```

**ARM64 implementation:** Direct port using ARM64 branch/compare/load instructions. The high-level pattern uses `InsertCompareBranch`, `InsertMove`, `InsertBranch` — all of which are architecture-independent helpers that produce the correct ARM64 instructions through the existing `LowerCondBranch` and `ChangeToAssign` machinery.

On non-Windows platforms the code always does the bounds check (no guard-page optimization), which simplifies the ARM64 path.

**Estimated lines:** ~80

---

### 5.7 `LowerAsmJsStElemHelper` — Effort: **Moderate**

Same pattern as `LowerAsmJsLdElemHelper` but for stores. Out-of-bounds stores are silently dropped (no default value assignment).

**Estimated lines:** ~70

---

### 5.8 `EmitFloat32ToFloat64` — Effort: **Trivial**

ARM64 has `FCVT` which is already in `MdOpCodes.h` and encoded by `EncoderMD.cpp`.

```c
// ARM64 implementation:
IR::Instr* cvt = IR::Instr::New(Js::OpCode::FCVT, dst, src, m_func);
instrInsert->InsertBefore(cvt);
```

**Estimated lines:** ~5

---

### 5.9 `EmitInt64toFloat` — Effort: **Trivial**

ARM64 has `SCVTF` (signed integer to float). The NEON encoder has `EmitNeonScvtf64`. The existing `EmitIntToFloat` (`LowerMD.cpp` L6671-6682) already uses `FCVTZ`/`FCVT` patterns — this is the reverse direction but same infrastructure.

Need to add `SCVTF` to `MdOpCodes.h` if not already present, or use the existing `FCVT` opcode variant.

**Estimated lines:** ~10

---

### 5.10 `EmitSignExtend` — Effort: **Trivial**

**AMD64 reference** (`LowerMDShared.cpp` L7031-7080): Uses `MOVSX` / `MOVSXW` / `MOVSXD`.

**ARM64 equivalent:** Use `SXTB`/`SXTH`/`SXTW` which are aliases of `SBFM`. The ARM64 backend already has `SBFX` in `MdOpCodes.h`. Can use `LDRS` (load signed) for memory operands or synthesize with `SBFX` for register operands.

Alternative: emit `MOV` with appropriately typed (signed, smaller) source operand — the existing `ChangeToAssign` infrastructure in the ARM64 lowerer handles sign-extended loads via `LDRS`.

**Estimated lines:** ~25

---

### 5.11 `EmitReinterpretPrimitive` — Effort: **Trivial**

**AMD64 reference** (`LowerMDShared.cpp` L7593-7660): Uses `MOVQ` (64-bit) or `MOVD` (32-bit) to bitwise-copy between GP and XMM registers.

**ARM64 equivalent:** `FMOV_GEN` — already in `MdOpCodes.h` (L126) and used in `SaveDoubleToVar` (`LowerMD.cpp` L5194). This instruction moves bits between GP registers and FP/NEON registers without conversion.

```c
// ARM64 implementation:
// 8-byte reinterpret (i64 ↔ f64):
IR::Instr::New(Js::OpCode::FMOV_GEN, dst, src, m_func);
// 4-byte reinterpret (i32 ↔ f32):
IR::Instr::New(Js::OpCode::FMOV_GEN, dst, src, m_func);
```

**Estimated lines:** ~20

---

### 5.12 `LowerExitInstrAsmJs` — Effort: **Moderate**

The AsmJs epilog is a simplified version of the JS epilog. The JS epilog is already implemented at `arm64/LowerMD.cpp` L1520-1655 (`LowerExitInstr`). The AsmJs variant:
- Does NOT restore JavaScript frame pointers
- Does NOT pop CallInfo
- DOES restore callee-saved registers
- DOES restore FP/NEON callee-saved registers
- DOES deallocate stack frame

Can be modeled directly from `LowerExitInstr` by removing the JS-specific frame handling while keeping the ARM64-specific register restore and `RET` sequence. The `ARM64StackLayout` class (L1053-1127) can be reused as-is.

**Estimated lines:** ~80

---

### 5.13 `GenerateCopysign` — Effort: **Moderate**

**AMD64 reference** (`LowerMDShared.cpp` L5370-5398):
```c
// x86: ANDPS reg0, absDoubleCst    (clear sign bit of src1)
//      ANDPS reg1, sgnBitDoubleCst (isolate sign bit of src2)
//      ORPS  reg0, reg1            (combine)
```

**ARM64 implementation options:**

**Option A — FMOV_GEN + integer bitops (simplest, no new opcodes):**
```
FMOV_GEN  x0, d0        ; move src1 bits to GP register
AND       x0, x0, #abs_mask   ; clear sign bit
FMOV_GEN  x1, d1        ; move src2 bits to GP register
AND       x1, x1, #sgn_mask   ; isolate sign bit
ORR       x0, x0, x1    ; combine
FMOV_GEN  d0, x0        ; move back to FP register
```

**Option B — NEON BIT/BIF (if NEON pipeline is wired):**
```
; BIT Vd, Vn, Vm — bitwise insert if true
; Use sign-bit mask in Vm, src2 in Vn, src1 in Vd
```

Option A is recommended as it uses only existing `MdOpCodes` entries.

**Estimated lines:** ~30

---

## 6. New MdOpCodes & Encoder Work

### New entries for `arm64/MdOpCodes.h`

```c
// Atomic load-acquire
MACRO(LDAR,       Reg2,       OpSideEffect,   UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(LDARB,      Reg2,       OpSideEffect,   UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(LDARH,      Reg2,       OpSideEffect,   UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)

// Atomic store-release
MACRO(STLR,       Reg2,       OpSideEffect,   UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
MACRO(STLRB,      Reg2,       OpSideEffect,   UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
MACRO(STLRH,      Reg2,       OpSideEffect,   UNUSED,   LEGAL_STORE,    UNUSED,   DS__)

// Data memory barrier
MACRO(DMB_ISH,    Empty,      OpSideEffect,   UNUSED,   LEGAL_NONE,     UNUSED,   D___)

// Signed convert int64 to float (if not already reachable via FCVT)
MACRO(SCVTF,      Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(UCVTF,      Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
```

### New cases in `arm64/EncoderMD.cpp` `GenerateEncoding()`

Each new MdOpCode needs a case in the main encoding switch. The encoding functions already exist in `ARM64Encoder.h`:

| MdOpCode | Encoder Function | Source |
|----------|------------------|--------|
| `LDAR` | `EmitLdar` / `EmitLdar64` | `ARM64Encoder.h` L4231-4249 |
| `LDARB` | `EmitLdarb` | `ARM64Encoder.h` L4209-4216 |
| `LDARH` | `EmitLdarh` | `ARM64Encoder.h` L4220-4227 |
| `STLR` | `EmitStlr` / `EmitStlr64` | `ARM64Encoder.h` L4378-4397 |
| `STLRB` | `EmitStlrb` | `ARM64Encoder.h` L4356-4365 |
| `STLRH` | `EmitStlrh` | `ARM64Encoder.h` L4367-4376 |
| `DMB_ISH` | `EmitDmb` | `ARM64Encoder.h` L740-745 |
| `SCVTF` | `EmitNeonScvtf` / `EmitNeonScvtf64` | `ARM64NeonEncoder.h` L4400-4424 |
| `UCVTF` | `EmitNeonUcvtf` / `EmitNeonUcvtf64` | `ARM64NeonEncoder.h` L4426-4450 |

**Estimated total encoder lines:** ~60

---

## 7. Memory Model Considerations

This is the most architecturally significant difference between x86 and ARM64 for WASM.

### x86 Memory Model (TSO — Total Store Order)
- All loads and stores are **implicitly ordered** (load-load, store-store, load-store)
- Only store-load reordering is allowed
- `MOV` is already atomic for aligned accesses
- Atomic store uses `XCHG` (full barrier) or `MOV` + `MFENCE`
- Atomic load uses `MOV` + `LOCK OR [rsp], 0` (or just `MOV` on x86)

### ARM64 Memory Model (Weakly Ordered)
- Loads and stores can be **freely reordered** by hardware
- Explicit barriers or acquire/release instructions needed for ordering
- **Key instructions:**
  - `LDAR` — Load-Acquire: prevents subsequent memory operations from being reordered before this load
  - `STLR` — Store-Release: prevents prior memory operations from being reordered after this store
  - `DMB ISH` — Data Memory Barrier (full inner-shareable barrier)
  - `DMB ISHLD` — Load barrier only
  - `DMB ISHST` — Store barrier only

### WASM Memory Model Mapping

WASM's shared memory model (for `memory.atomic.*` operations) follows the C++ memory model:

| WASM Operation | x86 Implementation | ARM64 Implementation |
|---------------|-------------------|---------------------|
| `i32.atomic.load` | `MOV` (implicit acquire on TSO) | `LDAR` |
| `i32.atomic.store` | `XCHG` (implicit full barrier) | `DMB ISH` + `STLR` |
| `i32.atomic.load8_u` | `MOVZX byte` | `LDARB` + zero-extend |
| `i32.atomic.store8` | `XCHG byte` | `DMB ISH` + `STLRB` |
| `atomic.fence` | `LOCK OR [rsp], 0` | `DMB ISH` |

The `LDAR`/`STLR` pair provides sequential consistency when used together, which matches the WASM atomic semantics. All required encoder functions already exist in `ARM64Encoder.h`.

---

## 8. WASM SIMD — Deferred Phase

WASM SIMD maps 128-bit SIMD operations to native vector instructions. On x86, this maps to SSE/SSE2/SSE4.1/AVX. On ARM64, this maps to NEON (Advanced SIMD).

### Current State

| Component | Status | Lines |
|-----------|--------|-------|
| x86 SIMD Lowerer | ✅ Complete | `LowerMDSharedSimd128.cpp` — 3,310 lines |
| ARM64 NEON Encoder | ✅ Complete | `ARM64NeonEncoder.h` — 5,022 lines, 400+ operations |
| ARM64 SIMD Lowerer | ❌ Missing | Needs ~2,000-3,000 lines |
| SIMD Opcode Mapping | ❌ Missing | ~150 WASM SIMD opcodes to map |

### Why It's Deferrable

1. WASM MVP does not require SIMD — it's a post-MVP extension
2. The core WASM functionality (memory, calls, control flow, scalar types) works without SIMD
3. WASM modules that don't use SIMD will work once the core port is done
4. The NEON encoder infrastructure is ready when we get to it

### NEON ↔ SSE Opcode Mapping (representative sample)

| WASM SIMD Op | x86 (SSE) | ARM64 (NEON) | Encoder Ready? |
|-------------|-----------|-------------|----------------|
| `i32x4.add` | `PADDD` | `ADD.4S` | ✅ `EmitNeonAdd` |
| `f32x4.mul` | `MULPS` | `FMUL.4S` | ✅ `EmitNeonFmul` |
| `v128.and` | `ANDPS` | `AND.16B` | ✅ `EmitNeonAnd` |
| `i8x16.shuffle` | `PSHUFB` | `TBL` | ✅ `EmitNeonTbl` |
| `i32x4.shr_s` | `PSRAD` | `SSHR.4S` | ✅ `EmitNeonSshr` |
| `f64x2.sqrt` | `SQRTPD` | `FSQRT.2D` | ✅ `EmitNeonFsqrt` |
| `v128.bitselect` | `PBLENDVB` | `BSL.16B` | ✅ `EmitNeonBsl` |

The mapping is generally straightforward. The main complexity is in:
- Immediate shuffle patterns (NEON `TBL`/`TBX` vs SSE `PSHUFB`/`PSHUFD`)
- Integer widening/narrowing operations (different instruction sets)
- Saturating arithmetic (NEON and SSE differ in some edge cases)

---

## 9. Enablement Steps

### Phase 1: Build Enablement & Core Stubs (P0)

**Step 1.1 — Enable `ASMJS_PLAT` for ARM64**

```diff
// lib/Common/CommonDefines.h L697-699
-#if (defined(_M_IX86) || defined(_M_X64)) && !defined(DISABLE_JIT)
+#if (defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM64)) && !defined(DISABLE_JIT)
 #define ASMJS_PLAT
 #endif
```

**Step 1.2 — Fix compile errors**

Enabling `ASMJS_PLAT` and `ENABLE_WASM` will cause many `#ifdef` blocks to compile for ARM64 for the first time. Expect:
- Missing includes in ARM64 build paths
- x86-specific code inside `ASMJS_PLAT` blocks that needs `#if` guards
- `InterpreterStackFrame` AsmJs interpreter thunk differences (stack layout)

**Step 1.3 — Add atomic `MdOpCodes`**

Add `LDAR`, `STLR`, `DMB_ISH`, `SCVTF`, `UCVTF` entries to `arm64/MdOpCodes.h`.

**Step 1.4 — Wire encoder**

Add encoding cases for new opcodes in `arm64/EncoderMD.cpp`.

### Phase 2: Core WASM Lowering (P0)

**Step 2.1 — Implement call lowering**
- `LowerAsmJsCallI` (~10 lines, uses existing `LowerCall`/`LowerCallArgs`)
- `LowerAsmJsCallE` (~10 lines, same)

**Step 2.2 — Implement bounds checking**
- `LowerWasmArrayBoundsCheck` (~50 lines, mostly architecture-independent helpers)
- `LowerAsmJsLdElemHelper` (~80 lines, bounds check + load pattern)
- `LowerAsmJsStElemHelper` (~70 lines, bounds check + store pattern)

**Step 2.3 — Implement atomics**
- `LowerAtomicStore` (~20 lines, `DMB ISH` + `STLR`)
- `LowerAtomicLoad` (~15 lines, `LDAR`)

**Step 2.4 — Implement type conversions**
- `EmitFloat32ToFloat64` (~5 lines, `FCVT`)
- `EmitInt64toFloat` (~10 lines, `SCVTF`)
- `EmitSignExtend` (~25 lines, `SBFX`/`SXTW`/`SXTH`/`SXTB`)
- `EmitReinterpretPrimitive` (~20 lines, `FMOV_GEN`)

**Step 2.5 — Implement frame/calling support**
- `LowerExitInstrAsmJs` (~80 lines, adapt from `LowerExitInstr`)
- `GenerateCopysign` (~30 lines, `FMOV_GEN` + `AND`/`ORR`)

### Phase 3: Testing & Stabilization (P0)

- Run the WASM spec test suite (`test/WasmSpec/`)
- Expect initial pass rate around 40-60%
- Debug and fix lowering issues
- Verify atomics correctness under contention

### Phase 4: WASM SIMD (P1 — separate effort)

- Create `arm64/LowerMDSimd128.cpp` mapping SSE ops to NEON
- Wire into `Lower.cpp` SIMD dispatch
- ~2,000-3,000 lines of new code
- Run WASM SIMD spec tests

---

## 10. Risk Assessment

| Risk | Level | Impact | Mitigation |
|------|-------|--------|------------|
| **Compile errors from `ENABLE_WASM` flip** | Medium | Build breaks | Incremental: enable `ASMJS_PLAT` first, fix errors, then tackle stubs one by one |
| **DarwinPCS for WASM calls** | Low | Wrong ABI | Already fixed globally for JS JIT — WASM uses same `LowerCall` path |
| **W^X for WASM code pages** | None | — | Already solved: `MAP_JIT` + `pthread_jit_write_protect_np` |
| **ARM64 weak memory ordering** | Medium | Atomics correctness | Use `LDAR`/`STLR` which provide acquire/release matching WASM spec |
| **AsmJs interpreter thunk on ARM64** | Medium | Stack misalignment, wrong args | `InterpreterStackFrame` AsmJs paths have platform-specific stack layout code; needs ARM64 adaptation |
| **WASM SIMD on NEON** | High (but deferrable) | Large surface area | Defer to Phase 4; NEON encoder complete, just mapping layer needed |
| **64-bit integer operations** | Low | i64 ops fail | ARM64 is natively 64-bit; `EmitIntToLong`, `EmitUIntToLong`, `EmitLongToInt` already work |
| **WASM exception handling** | Low | try/catch in WASM | setjmp/longjmp solution already working for JS JIT |
| **Fast virtual buffer** | None | — | `ENABLE_FAST_ARRAYBUFFER` is already excluded from ARM64; explicit bounds checks are the only path |

---

## 11. File Inventory

### Files to Modify

| File | Changes | Complexity |
|------|---------|------------|
| `lib/Common/CommonDefines.h` | Add `_M_ARM64` to `ASMJS_PLAT` | Trivial |
| `lib/Backend/arm64/MdOpCodes.h` | Add ~9 atomic/conversion opcodes | Trivial |
| `lib/Backend/arm64/EncoderMD.cpp` | Add ~9 encoding cases | Low |
| `lib/Backend/arm64/LowerMD.h` | Replace 17 `Assert(UNREACHED)` stubs with real declarations | Low |
| `lib/Backend/arm64/LowerMD.cpp` | Implement 12-15 functions (~500-600 lines) | **Medium-High** |
| `lib/Backend/CMakeLists.txt` | No change expected (ARM64 already compiles `LowerMD.cpp`) | None |

### Files That Gain Compilation (no changes needed)

These files are currently compiled out by `#ifdef ASMJS_PLAT` / `#ifdef ENABLE_WASM` but are architecture-independent:

| File | Lines | Notes |
|------|-------|-------|
| `lib/WasmReader/*.cpp` | ~4,443 | WASM binary parser/codegen — architecture-independent |
| `lib/WasmReader/*.h` | ~1,945 | WASM data structures — architecture-independent |
| `lib/Runtime/Language/AsmJsModule.cpp` | ~1,930 | AsmJs module validation — architecture-independent |
| `lib/Runtime/Language/AsmJsEncoder.cpp` | — | AsmJs encoder — may need ARM64 guards |
| `lib/Runtime/Language/InterpreterStackFrame.cpp` (AsmJs paths) | — | AsmJs interpreter — may need ARM64 stack adaptation |
| `lib/Backend/IRBuilderAsmJs.cpp` | — | IR building — architecture-independent |
| `lib/Backend/BackendOpCodeAttrAsmJs.cpp` | — | Opcode attributes — architecture-independent |

### Files to Monitor for x86 Assumptions

| File | Potential Issue |
|------|----------------|
| `lib/Runtime/Language/AsmJsEncoder.cpp` | May contain x86-specific register references |
| `lib/Runtime/Language/AsmJsUtils.cpp` | May have stack layout assumptions |
| `lib/Runtime/Language/InterpreterStackFrame.cpp` | AsmJs stack frame layout differs per platform |
| `lib/Backend/LowerMDSharedSimd128.cpp` | Entirely x86-specific — do NOT include in ARM64 build |

---

## 12. Estimated Effort

### Phase 1+2+3: Core WASM on ARM64

| Task | Lines | Days |
|------|-------|------|
| Enable flags + fix compile errors | ~50 changes | 2-3 |
| Add MdOpCodes + encoder wiring | ~80 new lines | 1 |
| Implement 7 WASM core lowering functions | ~250 new lines | 3-4 |
| Implement 6 type conversion functions | ~95 new lines | 1-2 |
| Implement 2 frame/calling functions | ~110 new lines | 2-3 |
| Testing & debugging | — | 5-8 |
| **Total Phase 1-3** | **~585 new lines** | **14-20 days** |

### Phase 4: WASM SIMD

| Task | Lines | Days |
|------|-------|------|
| Create ARM64 SIMD lowering layer | ~2,500 new lines | 10-15 |
| Opcode mapping table | ~500 new lines | 3-5 |
| SIMD-specific testing | — | 5-8 |
| **Total Phase 4** | **~3,000 new lines** | **18-28 days** |

### Grand Total

| | New Lines | Calendar Days |
|---|-----------|---------------|
| **Core WASM** | ~585 | 14-20 |
| **WASM SIMD** (optional) | ~3,000 | 18-28 |
| **Everything** | ~3,585 | 32-48 |

---

## Appendix A: Reference Architecture Comparison

```
 x86/x64 WASM Backend                    ARM64 WASM Backend (proposed)
 ─────────────────────                    ──────────────────────────────
 Lower.cpp (shared)            ───►       Lower.cpp (shared, same file)
       │                                        │
       ▼                                        ▼
 LowerMDShared.cpp             ───►       arm64/LowerMD.cpp
       │                                   (add WASM functions here)
       ▼                                        │
 amd64/LowererMDArch.cpp                        ▼
       │                                  arm64/EncoderMD.cpp
       ▼                                   (add atomic opcodes)
 amd64/EncoderMD.cpp                            │
       │                                        ▼
       ▼                                  ARM64Encoder.h
 X64Encode.h                              ARM64NeonEncoder.h
                                           (already complete)
```

Note the structural difference: x86/x64 has a three-layer architecture (`LowerMDShared` → `LowererMDArch` → `EncoderMD`), while ARM64 has a two-layer architecture (`LowerMD` → `EncoderMD`). This means the ARM64 WASM implementations go directly into `arm64/LowerMD.cpp` rather than through a separate arch file.

---

## Appendix B: WASM Test Suite Expected Coverage

| Test Category | Tests | Expected After Phase 1-3 | After Phase 4 |
|--------------|-------|--------------------------|---------------|
| Core (i32, i64, f32, f64) | ~120 | ~90% | ~90% |
| Memory operations | ~45 | ~85% | ~85% |
| Control flow | ~35 | ~95% | ~95% |
| Call/return | ~30 | ~85% | ~85% |
| Conversions | ~40 | ~80% | ~80% |
| Atomics (threads) | ~25 | ~70% | ~70% |
| SIMD | ~287 | 0% | ~75% |
| **Total** | **~582** | **~50% raw / ~75% excl. SIMD** | **~80%** |