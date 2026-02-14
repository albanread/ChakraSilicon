# PAUSED

## Status: PROJECT PAUSED (February 14, 2026)

**Project paused pending future specialized analysis.**

### Final State Summary
1. **Resolved:** Large argument lists (7+ params) work correctly (`fix_lowermd.patch`).
2. **Resolved:** JIT-to-C++ varargs calls (e.g. `indexOf`, `Math.max`) are successfully bridged via new trampolines in `LowerMD.cpp`.
3. **Unresolved:** The Interpreter-to-C++ bridge (`arm64_CallFunction.S`) causes a crash during TypedArray initialization (`min_ta.js`).
   - Feature: This only occurs when `ENABLE_NATIVE_CODEGEN` is on (`chjita64`), even though the crash happens in the interpreter entry point.
   - Attempted Fix: Fixed a stack buffer overflow in `StaticInterpreterThunk` (unconditional 64-byte write).
   - Result: Crash persists (SIGSEGV).
   - Validated: Full clean build confirmed the crash is reproducible and not a build artifact.

### Next Steps (When Resumed)
- Focus entirely on `arm64_CallFunction.S` and `StaticInterpreterThunk`.
- The failure is likely a subtle ABI violation in how `StaticInterpreterThunk` sets up the `JavascriptCallStackLayout` structure on the stack compared to what the C++ `InterpreterThunk` expects on macOS.
- Compare `chinta64` (working) vs `chjita64` (crashing) execution flow for `InitializeTypedArrayPrototype`.

---
## ARM64 JIT Call Bugs — Status for Review

**Date:** 2026-02-13 (updated late 2026-02-13)  
**Build:** `build/chjita64` (ARM64 JIT on Apple Silicon, DarwinPCS ABI)  
**Current status:** Bug B validated fixed. Bug A has new infrastructure (runtime-checking trampoline) covering all three indirect call paths. `GenerateDirectCall` was also wired to `HelperCallDirectVarargs` (2026-02-13 late), but `min_ta.js` and `test_varargs_crash.js` still crash. Timeout-safe lldb evidence continues to show an earlier crash during TypedArray prototype initialization through `arm64_CallFunction` (interpreter/C++ entry path), before any JIT call-split trace is emitted.

---

## Two Distinct Bugs

## New late finding (2026-02-13, timeout-safe lldb run)

To avoid hangs, lldb was run in batch mode with a hard timeout. This produced a stable backtrace.

Observed with:
- `build/chjita64/bin/ch/ch tests/repro_cases/test_varargs_crash.js`
- `build/chjita64/bin/ch/ch /tmp/min_ta.js` where `/tmp/min_ta.js` only does `new Int32Array(2)`

Results:
- `min_ta.js` on `chjita64` prints `start` then exits with `SIGSEGV`.
- The same script on `build/chinta64/bin/ch/ch` prints `start` + `made 2` (passes).
- lldb backtrace for `chjita64` crash tops out at:
  - `ValueType::FromObject`
  - `DynamicProfileInfo::RecordParameterInfo`
  - `InterpreterStackFrame::InterpreterThunk`
  - `arm64_CallFunction`
  - `JsBuiltInEngineInterfaceExtensionObject::InjectJsBuiltInLibraryCode`
  - `JavascriptLibrary::InitializeTypedArrayPrototype`

Interpretation:
- This indicates a failure path **before** the expected JIT hot-loop `CallDirect` varargs site.
- The crash occurs during TypedArray prototype/bootstrap call flow, not only during hot-loop helper dispatch.

High-confidence suspect introduced by recent changes:
- `lib/Runtime/Library/arm64/arm64_CallFunction.S` now unconditionally reloads `x2-x7` from stack slots after frame construction, even for low argument counts; this diverges from the Windows ARM64 reference trampoline (`arm64_CallFunction.asm`) which uses an argument-count-bounded register load path.

Action item from this finding:
- Investigate/fix `arm64_CallFunction.S` argument register load policy first (or in parallel with `GenerateDirectCall` trampoline wiring), then re-run `min_ta.js` and `test_varargs_crash.js` before concluding the remaining `CallDirect`-only gap.

Follow-up experiment results (same day):
- Tried a bounded register-load variant in `arm64_CallFunction` (load only existing `values[0..N-1]` into x2-x7 from `values[]` instead of unconditional stack reload). `min_ta.js` still crashed.
- Tried full overflow-only marshaling (Windows-style) in `arm64_CallFunction`. This regressed startup (`min_plain.js` crashed before output). Change was reverted.
- Wired `Lower.cpp::GenerateDirectCall` to route `CallDirect` through `HelperCallDirectVarargs` (real target moved to `x16`). Rebuild succeeded; `min_plain.js` still passes, but `min_ta.js` and `test_varargs_crash.js` still crash with `SIGSEGV`.
- Post-wiring timeout-safe lldb backtrace for `min_ta.js` remains in `JavascriptOperators::PatchGetValue` during TypedArray prototype/bootstrap via interpreter thunk and `arm64_CallFunction`.
- Current conclusion: `arm64_CallFunction` is still suspicious but not yet proven as the sole/root cause of the remaining crash path.

### Bug A: Varargs (C++ Entry* functions crash)

**Status:** Partially fixed. Indirect and CallDirect trampoline routing are now present, but crash persists in `test_varargs_crash.js` and in the minimal TypedArray bootstrap repro (`/tmp/min_ta.js`).

**Symptom:** `TypedArray.indexOf`, `TypedArray.filter`, etc. crash when called from JIT code.

**Root cause — PROVEN:**  
On DarwinPCS (Apple ARM64), variadic function arguments are **always passed on the stack**, never in registers. The JIT puts arguments in x0-x7 (registers). When a C++ `Entry*` function uses `va_start`, it reads from the caller's stack area — which contains garbage.

**Evidence (debug dump of JIT→indexOf call):**
```
x0 (function)  = 0x10431eb00     ← correct
x1 (callInfo)  = 0x2000002       ← correct [Count=2]
x2 (arg0/this) = 0x10431f840     ← correct
x3 (arg1)      = 0x100000000001e ← correct
Stack [SP+0]   = 0x102c994b8     ← GARBAGE (target addr, not function)
Stack [SP+8]   = 0x1029361bc     ← GARBAGE (code addr, not callInfo)
```
C++ va_start reads [SP+0] onward → crash.

**Fix — VALIDATED (for helper calls):**  
An assembly trampoline (`arm64_DebugTrampoline` in `arm64_CallFunction.S`) that:
1. Receives args in x0-x7 from JIT
2. Spills them to a contiguous stack frame: [function, callInfo, arg0, arg1, ...]
3. Calls the real C++ target via `blr x16`

When wired into LowerCall for helper calls, **TypedArray.indexOf passes.** This is the correct fix.

#### Understanding the call paths (code-reading, not speculation)

By reading `LowerMD.cpp`, `Lower.cpp`, and the assembly trampolines, we identified **four distinct paths** a JIT-emitted call can take. Each has different varargs exposure:

| # | Path | Opcode | Lowered by | How target is obtained | Trampoline coverage |
|---|------|--------|------------|----------------------|-------------------|
| 1 | **CallDirect** (helper) | `CallDirect` | `Lower.cpp` → `GenerateDirectCall` → `LowerCall` | `HelperCallOpnd` (address baked in at JIT time) | ✅ Routed via `HelperCallDirectVarargs` (`arm64_CallDirectVarargs`), with real target moved to `x16` before call. |
| 2 | **CallI (fixed native)** | `CallI` / `CallIFixed` | `LowerCallI` | `GeneratePreCall` loads `funcObj→type→entryPoint`. Target is **known at JIT time** via `HasFixedFunctionAddressTarget()` and resolved to a non-script function via `IsScriptFunction()`. | ✅ Routed through `arm64_CallIndirectMaybeVarargs` (unconditional contiguous frame). |
| 3 | **CallI (fixed script)** | `CallI` / `CallIFixed` | `LowerCallI` | Same as above, but `IsScriptFunction()` returns true. | ❌ Direct BLR — **correct**, script functions use the register/overflow convention. |
| 4 | **CallI (dynamic)** | `CallI` | `LowerCallI` | `GeneratePreCall` loads `funcObj→type→entryPoint`. Target is **unknown at JIT time** (`HasFixedFunctionAddressTarget()` is false). | ✅ Now routed through `arm64_CallIndirectCheckVarargs` (runtime script/native check). **Added 2026-02-13.** |
| 5 | **CallIDynamic** | `CallIDynamic` | `LowerCallIDynamic` | Same `GeneratePreCall`. Always dynamic. | ✅ Now routed through `arm64_CallIndirectCheckVarargs`. **Added 2026-02-13.** |

**Why we believe the paths are as described:** We read the actual code flow:
- `GeneratePreCall` (LowerMD.cpp ~line 660) always loads the entry point via two indirections: `LDR type, [funcObj + offsetOfType]` then `LDR entryPoint, [type + offsetOfEntryPoint]`. This is the same for all indirect calls.
- `HasFixedFunctionAddressTarget()` (Instr.cpp) returns true only when the optimizer has baked the function address into the IR as a constant `AddrOpnd` with `m_isFunction` set. Otherwise the target is "dynamic".
- The TypeId check **cannot** distinguish script from native — both use `TypeIds_Function` (27). This was confirmed by reading `ScriptFunctionType` constructor and `VarIsImpl<JavascriptFunction>`.

#### Why `test_varargs_crash.js` still crashes

`test_varargs_crash.js` and `/tmp/min_ta.js` still crash after CallDirect wiring.

Current evidence now favors an **earlier bootstrap/interpreter path** rather than a remaining CallDirect routing gap:
- CallDirect is now redirected through `HelperCallDirectVarargs`.
- `CHAKRA_TRACE_CALL_SPLIT=2` still shows no call-split lines before crash (consistent with failure before `LowerCallI`-instrumented sites execute).
- Timeout-safe lldb backtrace on `/tmp/min_ta.js` still lands in `JavascriptOperators::PatchGetValue` during TypedArray prototype initialization, with stack frames through `InterpreterStackFrame::InterpreterThunk` and `arm64_CallFunction`.

Updated hypothesis:
- The remaining crash is likely independent of the CallDirect varargs routing gap and is in an early initialization / interpreter-call path (possibly involving object/property bootstrap state or argument/stack handling around `arm64_CallFunction`).

#### Script vs native detection at runtime

For dynamic calls (paths 4 and 5), we cannot know at JIT compile time whether the callee is a script function or native. The new `arm64_CallIndirectCheckVarargs` trampoline solves this with a runtime check:

```
ldr x17, [x0, #0x28]      ; funcObj->functionInfo  (JavascriptFunction::GetOffsetOfFunctionInfo())
ldr x17, [x17, #0x08]      ; functionInfo->functionBodyImpl (FunctionInfo::GetFunctionBodyImplOffset())
cbz x17, native_path       ; NULL = native/builtin, non-NULL = script
```

**Why this check is correct:**
- `JavascriptFunction::IsScriptFunction()` is defined as `functionInfo->HasBody()` which is `functionBodyImpl != NULL`. We read this definition in `FunctionInfo.h`. Native/builtin functions have `functionBodyImpl = NULL` because they have no JS function body — they are implemented directly in C++.
- Offset 0x28 for `functionInfo`: Verified by reading the class hierarchy. `RecyclableObject` has vtable (0x00) + type (0x08) = 16 bytes. `DynamicObject` adds `auxSlots` (0x10) + `objectArray` (0x18) = 32 bytes. `JavascriptFunction` adds `constructorCache` (0x20) + `functionInfo` (0x28). This matches `JavascriptFunction::GetOffsetOfFunctionInfo()` which uses `offsetof()`.
- Offset 0x08 for `functionBodyImpl`: `FunctionInfo` has `originalEntryPoint` (0x00) + `functionBodyImpl` (0x08). This matches `FunctionInfo::GetFunctionBodyImplOffset()`.
- **Caveat:** These offsets are for release builds. Debug builds with `ENABLE_TTD_DEBUGGING` insert 16 extra bytes into `DynamicObject`, which would shift `functionInfo` to 0x38. A runtime assertion (`VerifyArm64TrampolineOffsets()`) aborts with a diagnostic message if the offsets don't match.

**Why tail-call (BR) is safe for the script path:** If `functionBodyImpl != NULL`, the target is a script function. Script functions use the normal JIT calling convention (args in x0-x7, overflow at `[CallerSP+0]`). The trampoline uses `br x16` (not `blr`) so it does NOT create a new frame, does NOT push FP/LR, and the callee sees the exact same SP and overflow layout as a direct call. This was confirmed by reading the ARM64 ISA: `BR` is an unconditional branch to register with no link — no state is modified except PC.

**Why the contiguous frame in the native path doesn't break:** The trampoline saves FP/LR, builds a contiguous frame `[function, callInfo, arg0, ...]` at its SP, then calls via `blr x16`. The native callee's `va_start` reads from its caller's SP, which is the trampoline's stack — exactly the contiguous frame. No JS overflow convention is involved because native Entry* functions never access `[CallerSP+0]` for overflow; they use `va_arg` exclusively.

**New observability (added):** `LowerCallI` now records split decisions and can emit end-of-process stats:
- `CHAKRA_TRACE_CALL_SPLIT=1` → summary counters at exit
- `CHAKRA_TRACE_CALL_SPLIT=2` → summary + per-call-site trace lines

Summary fields:
- total `LowerCallI` sites lowered
- fixed-function targets
- fixed script targets
- fixed native targets
- fixed unknown-class targets
- fixed targets routed to trampoline
- dynamic/non-fixed targets

**Important behavior:** if a repro hard-hangs and blocks process shutdown, `atexit()` does not run, so the summary is not printed. In that case use mode `2` to get live per-call-site lines before the hang.

---

### Bug B: 7+ parameter JS→JS calls crash

**Status:** Fixed (validated)

**Symptom:** Any JS function with 7+ parameters segfaults when JIT-compiled.

**Validation on current build (`build/chjita64/bin/ch/ch`):**
- `tests/repro_cases/test_arg_corruption.js` → `PASS: 7-param test`
- `tests/repro_cases/test_7p_900.js` → steady `28` results through iteration 899, final `DONE`

No hang or crash reproduced in these two 7-parameter repro cases after rebuild.

**Reproduction:**
```js
function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }
for (var i = 0; i < 2000; i++) test7p(1, 2, 3, 4, 5, 6, 7);
```
Segfault (exit 139). 6 parameters works fine.

**This is the ORIGINAL pre-existing bug.** It crashes with ZERO code changes on a clean build.

**What we know:**
- The JIT calling convention uses x0=function, x1=callInfo, x2-x7 for the first 6 values (this + 5 args). The 7th+ args overflow to `[CallerSP+0]`.
- The interpreter trampoline (`arm64_CallFunction`) builds a contiguous stack frame and works correctly for all arg counts.
- When the **callee** is JIT'd but the **caller** is interpreted, it works (result: 28, correct).
- The crash only happens when... we're not sure. It may require the caller to also be JIT'd, or it may be a callee-side frame layout issue.

**Callee JIT disassembly (captured via Capstone):**
```asm
sub sp, sp, #0xa0              ; allocate 160 bytes
; save callees: x19-x26, x29, x30
stp x0, x1, [x29, #0x10]      ; home x0,x1 at FP+16
stp x2, x3, [x29, #0x20]      ; home x2,x3 at FP+32
stp x4, x5, [x29, #0x30]      ; home x4,x5 at FP+48
stp x6, x7, [x29, #0x40]      ; home x6,x7 at FP+64
; ...
ldr x0, [sp, #0x68]           ; read callInfo from FP+0x18 (= homed x1)
; arg count check, then:
ldr x19, [sp, #0xa8]          ; ← arg "g" read from [SP+0xa8]
```
`[SP+0xa8]` = `[CallerSP+8]` (since frame is 0xa0). So the callee reads the 7th arg from `[CallerSP+8]`. The 7th arg should be at `[CallerSP+0]` (the first overflow slot). **Off by 8 bytes? Or CallerSP includes something else?**

Actually: frame = 0xa0 = 160 bytes. `[SP+0xa8]` = `[FrameBase+0xa8]` = `[CallerSP + 8]`. That means the callee expects arg "g" at CallerSP+8, not CallerSP+0. Need to verify what the caller puts at CallerSP+0 vs +8.

**Observation:** Enabling `CHAKRA_TRACE_JIT_ASM=1` or running under `lldb` prevents the crash — the timing change prevents JIT compilation of the caller, so the interpreter handles the call correctly.

**Not yet investigated:**
- What `GetOpndForArgSlot` produces for slot indices >= 8 (overflow args)
- Whether `m_argSlotsForFunctionsCalled` accounts for the full arg count
- Whether `ARM64StackLayout` computes `m_ArgumentsOffset` correctly for overflow reads
- The actual JIT'd caller code (never captured — caller doesn't get JIT'd when tracing is on)

---

## What Was Tried (and Why It Failed)

| Approach | Result | Why |
|----------|--------|-----|
| Remove Darwin shadow stores | Broke varargs (Entry* functions crash) | va_start needs args on stack |
| Add shadow stores + overflow offset | Fixed varargs, broke 7p JS→JS calls | Offset shifted overflow args to wrong location for JS callees |
| Trampoline on ALL calls (indirect + helper) | Fixed varargs, broke JS→JS calls | Trampoline interposes a frame (saves FP/LR → new CallerSP); JS callee reads overflow from `[CallerSP+0]` which is now the trampoline's data, not the real overflow arg |
| Trampoline on HELPER calls only | Fixed varargs for helpers; 7p still crashes | 7p crash is pre-existing Bug B, unrelated to trampoline |
| No changes (clean baseline) | 7p crashes, varargs crash | Both bugs exist in the original code |
| Runtime-checking trampoline on dynamic CallI + CallIDynamic | Builds OK, dynamic JS→JS calls pass | BUT `test_varargs_crash.js` still crashes — because `indexOf` likely goes through CallDirect (path 1), not CallI (path 4). See analysis above. |

---

## Infrastructure Built

All present in the codebase:

1. **`arm64_DebugTrampoline`** in `arm64_CallFunction.S`: Assembly trampoline that spills x0-x7 to a contiguous stack frame and calls target via x16. Also calls `debug_dump_jit_call` for diagnostics (controlled by env var). Available but not wired for production use.

2. **`arm64_DebugDump.cpp`**: C debug function. Captures call snapshots (regs, stack) into an in-memory ring buffer (4096 entries). Prints at exit via `atexit()` — zero I/O during hot path, does not disturb JIT timing.

3. **`arm64_CallDirectVarargs`** in `arm64_CallFunction.S`: Trampoline specifically for CallDirect varargs path. Registered as `HelperCallDirectVarargs` in helper tables and now wired from `GenerateDirectCall` (real target moved to `x16`). This did not resolve the remaining `min_ta.js` / `test_varargs_crash.js` crash.

4. **`arm64_CallIndirectMaybeVarargs`** in `arm64_CallFunction.S`: Trampoline for fixed-function native indirect calls. Unconditionally builds contiguous frame. **Wired** into `LowerCallI` for fixed native targets.

5. **`arm64_CallIndirectCheckVarargs`** in `arm64_CallFunction.S`: **NEW (2026-02-13).** Runtime-checking trampoline for dynamic indirect calls. Checks `functionBodyImpl` to distinguish script (tail-call, zero overhead) from native (contiguous frame). **Wired** into `LowerCallI` for dynamic targets and `LowerCallIDynamic`. Registered as `HelperCallIndirectCheckVarargs`.

6. **`Lower.cpp` (GenerateDirectCall)**: Apple ARM64 path now routes `CallDirect` through `HelperCallDirectVarargs` and passes the original target in `x16`, replacing the previous local shadow-store workaround.

7. **`JnHelperMethodList.h` / `JnHelperMethod.h`**: Three Apple ARM64 helpers registered: `HelperCallDirectVarargs`, `HelperCallIndirectMaybeVarargs`, `HelperCallIndirectCheckVarargs`.

8. **`CMakeLists.txt`**: `arm64_DebugDump.cpp` added to build.

9. **`LowerMD.cpp` (Apple ARM64 call-split tracing):** Env-driven call-site classification:
  - `CHAKRA_TRACE_CALL_SPLIT=1`: exit summary
  - `CHAKRA_TRACE_CALL_SPLIT=2`: verbose per-call + summary
  - Covers `LowerCallI` and `LowerCallIDynamic` only (does NOT cover CallDirect).

10. **`LowerMD.cpp` (offset verification):** `VerifyArm64TrampolineOffsets()` runs once on first JIT compilation. Compares the hardcoded assembly offsets (0x28 for `functionInfo`, 0x08 for `functionBodyImpl`) against `offsetof()` values from C++ headers. Aborts with a diagnostic message on mismatch.

LowerMD.cpp `LowerCall` is currently **clean** — no trampoline, no shadow stores, no offset changes. All Apple-specific routing is in `LowerCallI` (before `LowerCall` is called) and in `LowerCallArgs` (shadow stores for CallDirect only).

---

## What Needs To Happen

### For Bug A (varargs):

**Indirect calls (CallI) — DONE (believed complete):**
- Fixed native targets → `arm64_CallIndirectMaybeVarargs` (unconditional contiguous frame). Already validated.
- Fixed script targets → direct BLR (correct, no trampoline needed).
- Dynamic targets → `arm64_CallIndirectCheckVarargs` (runtime check). Added 2026-02-13. Builds and passes simple dynamic call tests. Not yet validated against a TypedArray repro that goes through this path.
- `LowerCallIDynamic` → `arm64_CallIndirectCheckVarargs`. Added 2026-02-13.

**Helper/CallDirect calls — now routed:**

`GenerateDirectCall` now redirects through `HelperCallDirectVarargs` (x16 holds the original target), matching the intended trampoline design.

**Result:** No improvement for the remaining crash repros (`/tmp/min_ta.js`, `test_varargs_crash.js`).

**Revised next step:** Focus investigation on the early TypedArray/bootstrap crash path observed in lldb (`PatchGetValue`/interpreter thunk/`arm64_CallFunction`), since this path fails before call-split traces and persists despite indirect + CallDirect trampoline routing.

### For Bug B (7+ args): RESOLVED
No blocker remains for Bug B on the current branch.

Current expectation:
- Keep the existing 7+ arg repros in regular regression runs.
- Treat any new 7+ arg crash/hang as a regression against the validated fix.

Regression coverage (current):
- `tests/repro_cases/test_arg_corruption.js`
- `tests/repro_cases/test_7p_900.js`

---

## Files of Interest

| File | Purpose |
|------|---------|
| `lib/Backend/arm64/LowerMD.cpp` | `LowerCall`, `LowerCallI`, `LowerCallIDynamic`, `GetOpndForArgSlot`, `LowerCallArgs` (shadow stores), `GeneratePreCall` (loads entryPoint), call-split tracing, offset verification |
| `lib/Backend/Lower.cpp` | `GenerateDirectCall` (CallDirect path — shadow stores, NOT trampoline). Line ~12308. |
| `lib/Runtime/Library/arm64/arm64_CallFunction.S` | All assembly: `arm64_CallFunction`, `arm64_CallJIT`, `arm64_CallDirectVarargs`, `arm64_CallIndirectMaybeVarargs`, `arm64_CallIndirectCheckVarargs`, `arm64_DebugTrampoline`, `StaticInterpreterThunk` |
| `lib/Runtime/Library/arm64/arm64_DebugDump.cpp` | Ring buffer debug dump (for `arm64_DebugTrampoline`) |
| `lib/Runtime/Language/Arguments.h` | `DECLARE_ARGS_VARARRAY`, `CALL_ENTRYPOINT` macros (where `va_start` is used) |
| `lib/Backend/JnHelperMethodList.h` | Helper method registrations (`CallDirectVarargs`, `CallIndirectMaybeVarargs`, `CallIndirectCheckVarargs`) |
| `lib/Backend/JnHelperMethod.h` | `extern "C"` declarations for the three Apple ARM64 helper trampolines |
| `lib/Runtime/Library/JavascriptFunction.h` | `GetOffsetOfFunctionInfo()` — offset 0x28 used by runtime check trampoline |
| `lib/Runtime/Base/FunctionInfo.h` | `GetFunctionBodyImplOffset()` — offset 0x08 used by runtime check trampoline; `HasBody()` = `functionBodyImpl != NULL` |

## Object Layout Reference (Release Build, 64-bit ARM64)

These offsets are used by the assembly trampolines and verified at runtime by `VerifyArm64TrampolineOffsets()`.

```
RecyclableObject:
  +0x00  vtable pointer (8 bytes, implicit from C++ virtual)
  +0x08  Type* type

DynamicObject (extends RecyclableObject):
  +0x10  Var* auxSlots / int16 inlineSlotSize + int16 offsetOfInlineSlots
  +0x18  ArrayObject* objectArray

JavascriptFunction (extends DynamicObject):
  +0x20  ConstructorCache* constructorCache
  +0x28  FunctionInfo* functionInfo          ← arm64_CallIndirectCheckVarargs reads here

FunctionInfo (standalone, not a RecyclableObject):
  +0x00  JavascriptMethod originalEntryPoint
  +0x08  FunctionProxy* functionBodyImpl     ← NULL for native/builtin, non-NULL for script
  +0x10  LocalFunctionId functionId
  +0x14  uint compileCount
  +0x18  Attributes attributes

Type:
  +0x00  TypeId typeId (4 bytes)
  +0x08  JavascriptLibrary* lib
  +0x10  RecyclableObject* prototype
  +0x18  JavascriptMethod entryPoint         ← GeneratePreCall loads this for BLR target
```

**Warning:** Debug builds with `ENABLE_TTD_DEBUGGING` insert 16 bytes into `DynamicObject` before `auxSlots`, shifting ALL downstream offsets (e.g., `functionInfo` moves from 0x28 to 0x38). The runtime assertion catches this at startup.

---

## Research (2026-02-14 session recap)

### Bug B (7+ parameter JS→JS calls): SOLVED

Validated fixed on the current branch. `test_arg_corruption.js` and `test_7p_900.js` both pass reliably. No further action needed — keep as regression tests.

### Bug A (Varargs / C++ Entry* crashes): Partially solved, remaining crash isolated

#### What was proven (high confidence)

1. **Root cause of the varargs ABI mismatch is confirmed:** On DarwinPCS, variadic arguments after `...` are **always on the stack**, never in registers. The JIT places them in x0-x7. When C++ `Entry*` functions call `va_start`, they read garbage from the stack. This is definitively proven with register/stack dumps.

2. **The trampoline fix is architecturally correct.** Building a contiguous stack frame `[function, callInfo, arg0, arg1, ...]` before calling the real target makes `va_start` work. This was validated for helper calls — `TypedArray.indexOf` passes when the trampoline is wired in.

3. **All five JIT call paths are now covered with appropriate trampolines:**

   | Path | Trampoline | Status |
   |------|-----------|--------|
   | CallDirect (helper) | `arm64_CallDirectVarargs` | ✅ Wired |
   | CallI fixed native | `arm64_CallIndirectMaybeVarargs` | ✅ Wired |
   | CallI fixed script | Direct BLR (correct — no trampoline needed) | ✅ |
   | CallI dynamic | `arm64_CallIndirectCheckVarargs` | ✅ Wired |
   | CallIDynamic | `arm64_CallIndirectCheckVarargs` | ✅ Wired |

4. **The runtime script/native check is correct.** `functionInfo->functionBodyImpl == NULL` means native/builtin. Non-NULL means script. Object layout offsets verified: `functionInfo` at +0x28, `functionBodyImpl` at +0x08 (release builds). A runtime assertion (`VerifyArm64TrampolineOffsets`) guards against layout changes.

#### What the remaining crash IS

Despite full trampoline coverage, `min_ta.js` (`new Int32Array(2)`) and `test_varargs_crash.js` still crash with SIGSEGV. The crash is **NOT in a JIT call-split path**:

- `CHAKRA_TRACE_CALL_SPLIT=2` emits zero trace lines before the crash.
- lldb backtrace shows: `PatchGetValue` → `InterpreterThunk` → **`arm64_CallFunction`** → `InjectJsBuiltInLibraryCode` → `InitializeTypedArrayPrototype`.
- This is an **interpreter/bootstrap** path during TypedArray prototype initialization.
- The interpreter-only build (`chinta64`) passes the same script — so JIT being enabled changes something about this bootstrap path.

#### What was tried for `arm64_CallFunction.S` and failed

| Experiment | Result |
|-----------|--------|
| Bounded register load (load only existing values[0..N-1] into x2-x7) | Still crashed |
| Full overflow-only (Windows-style) marshaling | Regressed — `min_plain.js` crashed before output; reverted |

#### Key insight for next session

The remaining crash is in `arm64_CallFunction` (the **interpreter** trampoline, not the JIT trampolines we built). It occurs during early library bootstrap, before any user JavaScript runs. The crash path is:

```
InitializeTypedArrayPrototype → InjectJsBuiltInLibraryCode → arm64_CallFunction 
→ InterpreterThunk → PatchGetValue → SIGSEGV
```

This suggests either:
- `arm64_CallFunction` is corrupting something during the contiguous-frame build + unconditional x2-x7 reload for bootstrap interpreter calls, OR
- JIT being enabled changes the bootstrap code path in a way that triggers a latent bug in the interpreter/property system.

The fact that `chinta64` (interpreter-only) passes the same script is the critical clue — something about having JIT enabled changes behavior even before JIT compilation happens.

#### Recommended next step

Compare `chinta64` vs `chjita64` execution of `new Int32Array(2)` at the `arm64_CallFunction` level — specifically what happens during `InitializeTypedArrayPrototype`. The JIT build may be taking a different initialization path or calling a different entry point that exposes the `arm64_CallFunction` contiguous-frame issue in a way the interpreter-only build does not.

---

## Systemic Analysis: Windows vs. macOS ARM64 Divergence

### 1. The ABI Gulf: Registers vs. Stack
The fundamental "lost in translation" error occurs because ChakraCore assumes the **Windows ARM64 ABI** (where `x0-x7` carry the first 8 arguments regardless of function type) is the universal truth.
- **Windows / Linux (AAPCS64):** A variadic function `(int, ...)` accepts registered parameters in `x0-x7`. The callee (the C++ function) is responsible for spilling them to the stack if it uses `va_start`.
- **macOS / Apple Silicon (DarwinPCS):** A variadic function **ignores** `x0-x7` for variadic arguments. It expects them **pre-spilled** on the stack at `[SP+0]`, `[SP+8]`, etc.

### 2. The Codebase Assumption
`LowerMD.cpp` is built around an `ArgumentReader` / `LowerCallArgs` loop that linearizes arguments into register operands. It does not natively support "For this specific call target, put N arguments on the stack, *starting at offset 0*." It always wants to fill the registers first. Refactoring `LowerMD.cpp` to understand DarwinPCS natively would require rewriting the register allocator and argument lowering logic—a high-risk change.

### 3. The Safe "Translation" Strategy: Middleware Trampolines
Since we cannot easily change how the JIT *emits* calls (filling registers), we must change how the calls are *delivered*.
- **The Protocol:** JIT continues to emit Windows-style calls (args in regs).
- **The Translation:** A thin assembly shim (trampoline) intercepts the call. It reads the registers (JIT convention) and writes them to the stack (DarwinPCS convention).
- **The Target:** The C++ Runtime sees a perfectly compliant DarwinPCS stack frame.

### 4. What Needs to be Refactored for macOS
Instead of patching `LowerMD.cpp` with `#ifdef __APPLE__`, we should formalize a **"Cross-ABI Bridge"**:
1.  **Helper Calls (Static):** These are easiest. In `JnHelperMethodList.h`, map every variadic helper to a trampoline. `LowerMD.cpp` doesn't need to know; it just calls the address it's given. The current fix (HelperCallDirectVarargs) does exactly this.
2.  **Indirect Calls (Dynamic):** This is the hard part. The JIT doesn't know *at compile time* if the target is a C++ Runtime function (requires stack args) or another JIT'd JS function (requires reg args).
    *   *Solution:* The **Runtime-Checking Trampoline** (`arm64_CallIndirectCheckVarargs`) introduced in this session is the correct systemic fix. It acts as a dynamic ABI adapter.

### 5. The Interpreter Anomaly (`arm64_CallFunction`)
Why does `chjita64` crash in the interpreter?
- `arm64_CallFunction` is the interpreter's way of calling C++ Runtime functions.
- On Windows, it fills `x0-x7` *and* puts overflow on the stack.
- On macOS, it *must* put everything on the stack. The current assembly does this (builds a contiguous frame).
- **The Suspect:** The crash in *TypedArray* initialization suggests that `arm64_CallFunction` might be blindly loading garbage into `x0-x7` from the stack it just built (unconditional load), or possibly the stack alignment/padding logic (`add x5, x2, #3` / `lsr x5, x5, #1`) is slightly off for specific argument counts (like 0 or 1 arg), creating a misaligned pointer that `va_start` chokes on, *or* the JIT library initialization logic triggers a different code path (e.g. `OP_NewScObj` vs `OP_NewScObj_JIT`) that invokes `arm64_CallFunction` with a signature that doesn't match the expectation.

## Deep Dive: The ABI Divergence (x64 vs ARM64)

### Why does this work on x64 (Intel) macOS?
You might wonder why ChakraCore on x64 doesn't suffer from this same "Windows vs. macOS" confusion.
1.  **Obvious Incompatibility:** On x64, the difference between Windows and macOS (System V ABI) is massive and visible.
    *   **Windows x64:** Args in `RCX`, `RDX`, `R8`, `R9`.
    *   **macOS x64:** Args in `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`.
    *   Because the registers are totally different, the backend *must* implement explicit logic for each platform. You can't accidentally reuse Windows logic on macOS; it simply wouldn't compile or run for *any* function. The codebase handles this via `UnixCall` vs `amd64_CallFunction`.
2.  **Variadic Similarity:** Crucially, on x64 macOS, variadic arguments **ARE** passed in registers (if available). The `va_start` mechanism relies on the callee saving registers to a "register save area". It does *not* require the caller to force arguments onto the stack.

### The "Trap" of ARM64
On ARM64, the situation is deceptive:
1.  **Standard Calls are Identical:** Both Windows and macOS use `x0`-`x7` for standard (non-variadic) function calls. The JIT backend can reuse 95% of the Windows logic for macOS, and it works perfectly for normal JS functions.
2.  **The Variadic Split:** The divergence happens *only* for variadic functions (like `EntryIndexOf(...)`).
    *   **Windows / Standard ARM64:** "Keep using `x0`-`x7`. The callee will figure it out."
    *   **Apple ARM64:** "Stop. For variadics, you MUST skip the registers and push to the stack."
3.  **The Impact:** Because standard calls work, the system seems healthy. But when the JIT optimizes a call to a built-in function (which is often variadic to handle variable JS arguments), it inadvertently uses the "Standard Call" convention (registers). This works on Windows. On macOS, it's a violation of the contract. The C++ helper looks at the stack (which is empty/garbage) and crashes.

**Summary:** x64 forces you to account for differences immediately. ARM64 lulls you into a false sense of security with compatible standard calls, then breaks specifically on interactions between JIT code and Variadic C++ Runtime helpers.

## Research Audit (Variadic Functions)

### Hypothesis
The user asked: *"if this issue ONLY relates to variadic functions we should be able to see where those functions are audit the project for all variadic functions."*

### Findings
The audit confirms that **virtually all** core JavaScript built-ins are implemented as variadic C++ functions in the Runtime, regardless of whether the JavaScript method they implement takes fixed arguments (like `Date.now()`) or variable arguments (like `Math.max()`).

This is because the `CallInfo` structure (argument count) is passed alongside the arguments, and the standard pattern for a built-in entry point is:
```cpp
Var EntrySomeMethod(RecyclableObject* function, CallInfo callInfo, ...);
```
Inside the function, the macro `ARGUMENTS(args, callInfo)` (which expands to `DECLARE_ARGS_VARARRAY`) uses `va_start` to access the arguments.

### Audit List (Sample of Affected Areas)
A comprehensive search for `ARGUMENTS`, `RUNTIME_ARGUMENTS`, and `CallInfo ..., ...` signatures reveals the scope is effectively "The Entire JS Runtime Library":

1.  **Core Objects (GlobalObject.cpp, JavascriptObject.cpp):**
    *   `EntryParseInt`, `EntryParseFloat`, `EntryIsNaN`, `EntryIsFinite`
    *   `EntryEval`, `EntryCollectGarbage`
2.  **Array & String (JavascriptArray.cpp, JavascriptString.cpp):**
    *   `EntryIndexOf`, `EntryIncludes`, `EntrySlice`, `EntrySplice`, `EntryJoin`
    *   `EntryCharAt`, `EntryCharCodeAt`, `EntryConcat`, `EntryMatch`, `EntryReplace`
3.  **Math & Number (JavascriptMath.cpp, JavascriptNumber.cpp):**
    *   `EntryAbs`, `EntryCos`, `EntrySin` (even simple 1-arg math functions are wrapped as variadic Entry points!)
    *   `EntryToFixed`, `EntryToPrecision`
4.  **Date (JavascriptDate.cpp):**
    *   `EntryNow`, `EntryParse`, `EntryUTC`
    *   All getters/setters (`EntryGetFullYear`, `EntrySetDate`, etc.)
5.  **TypedArrays (TypedArray.cpp):**
    *   `EntrySet`, `EntrySubarray`
    *   All %TypedArray% prototype methods relate back to the generic `Array` or `TypedArray` implementations using the same convention.

### Conclusion
The scope of the ABI mismatch is **system-wide** for the JIT-to-Runtime boundary. Any JIT optimization (CallDirect or CallI) that targets an `Entry*` function is vulnerable. The fix must be applied universally to all such calls, not just specific "known variadic" ones. The "Variadic Fix" is effectively the "Runtime Call Fix".

## The Logical Fix: A Universal Bridge

### The Problem in One Sentence
The JIT speaks **Windows ARM64** (arguments in registers), but the C++ Runtime expects **DarwinPCS ARM64** (arguments on stack) for all variadic functions.

### The Solution: The "Runtime Call Fix" Protocol
To fix this without rewriting the entire JIT backend or the entire C++ Runtime, we implement a **Universal Bridge** protocol using trampolines.

**1. The JIT Side (Sender):**
*   **Behavior:** Continues to assume the Windows ABI. It fills registers `x0`-`x7` with arguments.
*   **Change:** Instead of calling the C++ function address directly, it calls a **Trampoline Address**.
*   **Logic:** `CALL(Trampoline, RealTarget)`

**2. The Trampoline (Translator):**
*   **Input:** Receives `x0`-`x7` (Garbage in Darwin terms) and usually the `RealTarget` in a scratch register (like `x16`).
*   **Action:**
    1.  **Intercepts** execution before the C++ function runs.
    2.  **Spills** `x0`-`x7` onto the stack in a contiguous block (imitating what the C++ compiler *would* have done if the caller were standardized).
    3.  **Forwards** execution to the `RealTarget`.
*   **Output:** A stack frame that looks exactly like a compliant DarwinPCS call.

**3. The Runtime Side (Receiver):**
*   **Behavior:** Unchanged.
*   **Logic:** `va_start` looks at the stack. It finds the arguments placed there by the trampoline. everything works.

### Why this is the "Right" Fix
*   **Minimal Risk:** It touches zero logic inside the complex register allocator or the complex C++ runtime methods.
*   **Complete Coverage:** By routing *all* JIT-to-Runtime calls through this bridge, we solve the ABI mismatch for every single built-in function (Array, String, Math, etc.) at once.
*   **Performance:** The cost is a few instructions (store registers to stack) per runtime call, which is negligible compared to the runtime function's own logic.

### Did We Try This? (Status Check)
**Yes, partially.** We implemented the "Universal Bridge" strategy in segments:
1.  **Fixed Native Calls:** Routed through `arm64_CallIndirectMaybeVarargs`. (Done & Validated)
2.  **Helper Calls:** Routed through `HelperCallDirectVarargs`. (Done & Validated for `indexOf`, but revealed *new* crashes)
3.  **Dynamic Calls:** Routed through `arm64_CallIndirectCheckVarargs`. (Done & implemented)

**The Missing Piece:**
The **Interpreter's Bridge** (`arm64_CallFunction`) is the one failing now.
*   When the JIT is enabled, the interpreter uses `arm64_CallFunction` to call C++ helpers (like `InitializeTypedArrayPrototype`).
*   This function *tries* to be a bridge (it builds a stack frame), but it seems to be failing during the bootstrap phase, possibly due to:
    *   **Register Reloading Bug:** It unconditionally reloads `x0`-`x7` from the stack it just built, which might be corrupting them if the stack build was imperfect.
    *   **Arg Count Mismatch:** If called with 0 arguments (common in initialization), the stack math might be slightly off (padding/alignment), leading to a misaligned pointer for `va_start`.

**Conclusion:** We successfully implemented the bridge for **JIT-generated code**, but the crash moved to the **Interpreter's existing bridge** code (`arm64_CallFunction`), which is used heavily during startup when JIT is enabled. Fixing `arm64_CallFunction` is the final step to making the "Universal Bridge" complete.

### The Final Verdict: Will fixing `arm64_CallFunction` fix the problem?
**Yes, it is the last major blocker.**

1.  **Direct Evidence:** The current crash (SIGSEGV in `PatchGetValue`) happens *inside* the execution path controlled by `arm64_CallFunction`. It stops the engine from even starting up fully (bootstrapping TypedArrays).
2.  **Coverage:**
    *   **JIT Code:** Covered by our new trampolines (`HelperCallDirectVarargs`, etc.).
    *   **Interpreter/Bootstrap Code:** Covered by `arm64_CallFunction`.
3.  **The Outcome:** Once `arm64_CallFunction` correctly handles the DarwinPCS stack requirements for all argument counts (including the 0-arg internal calls used during initialization), the engine should successfully boot. At that point, our JIT trampolines will take over for the user code, and the system should run.

**Caveat:** "Fixing" it implies making it strictly compliant with DarwinPCS (always valid stack frame, perfect alignment) without regressing the Windows path (if shared) or breaking the delicate interpreter-to-JIT transitions.

### Clarification: Who calls `arm64_CallFunction`?
The user asked: *"the jit and the interpreter both call arm64_CallFunction to call built in javascript routines?"*

**Answer: No, mostly the Interpreter.**

1.  **The Interpreter (HEAVY usage):**
    *   The interpreter is written in C++. When it needs to execute a JavaScript function (or a Built-in like `Array.indexOf`), it cannot just "jump" there because it needs to set up the registers and stack according to the JS Calling Convention.
    *   It calls `arm64_CallFunction` to do this dirty work: "Please take these arguments from my C++ variables, put them into registers/stack, and run the target."
    *   **Crucially:** During startup (bootstrap), *everything* runs in the interpreter. The engine "interprets" the initialization scripts that set up `TypedArray`, `Promise`, etc. This is why `min_ta.js` crashes early—it's executing initialization code via `arm64_CallFunction`.

2.  **The JIT (RARE/NO usage):**
    *   The JIT *emits* (generates) its own machine code.
    *   When JIT'd code calls a built-in, it does **not** call `arm64_CallFunction`. It emits a `BL` (Branch with Link) instruction directly to the target address.
    *   **This is why we need trampolines:** Since the JIT generates the call instruction itself, we must interpose that generated call with a trampoline (e.g., `HelperCallDirectVarargs`) to fix the ABI on the fly.

**Why does `chjita64` crash in `arm64_CallFunction` if that's an interpreter tool?**
Because even in a JIT-enabled build, the engine starts in Interpreter mode. The JIT only kicks in later for "hot" functions. The crash happens so early (during bootstrap) that the JIT hasn't even compiled anything yet. The mere *presence* of JIT configurations or flags might be altering the bootstrap path (e.g., initializing JIT helpers), causing the interpreter to use `arm64_CallFunction` in a slightly different way (e.g., different argument counts) that triggers the latent DarwinPCS bug in that assembly file.

---
**Next Steps (Implementation Plan):**
1.  **Audit:** Confirm all `CallDirect` and `CallI` sites in `LowerMD.cpp` are covered by a trampoline routing.
2.  **Verify:** Ensure no "direct" calls to `Entry*` functions slip through without a trampoline.
3.  **Debug:** Solve the interpreter bootstrap crash (which is likely a "trampoline-like" issue in `arm64_CallFunction`).

## Code Analysis: `arm64_CallFunction` (The Interpreter Bridge)

### Previous Hypothesis (RETRACTED)

The earlier analysis claimed that the unconditional `ldp x2-x7` reload (loading FP/LR as garbage into argument registers when `argCount < 6`) was the "Smoking Gun." **This is incorrect**, for two reasons:

1.  **The callee doesn't blindly use those registers.** `arm64_CallFunction` calls `StaticInterpreterThunk` (assembly, line 518 of `arm64_CallFunction.S`), which builds its *own* `JavascriptCallStackLayout` from `x0-x7`, then passes it to the C++ `InterpreterThunk(layout)`. The C++ code reads `layout->callInfo.Count` to know how many args are valid. `InitializeParams` iterates only up to `min(inSlotsCount, requiredInParamCount)`. Garbage values beyond that are never accessed.

2.  **The fix was already tried and failed.** The status doc records: *"Tried a bounded register-load variant in arm64_CallFunction (load only existing values[0..N-1] into x2-x7). min_ta.js still crashed."* This conclusively proves the unconditional LDP is NOT the root cause.

### Corrected Analysis: The Real Crash Path

**Crash backtrace (from lldb):**
```
ValueType::FromObject → DynamicProfileInfo::RecordParameterInfo → InterpreterThunk 
→ arm64_CallFunction → InjectJsBuiltInLibraryCode → InitializeTypedArrayPrototype
```

**Key insight: `RecordParameterInfo` only exists in JIT-enabled builds.**

`RecordParameterInfo` is called at `InterpreterStackFrame.cpp:1406` when `profileParams` is true. This is gated by `ShouldDoProfile()`, which returns true when `ExecutionMode == ProfilingInterpreter`. **The interpreter-only build (`chinta64`) never enters this path** because it doesn't have `ENABLE_NATIVE_CODEGEN`, so profiling is compiled out.

This explains the central mystery: *why does `chinta64` pass but `chjita64` crash on the same script, in the same interpreter code?* It's not because `arm64_CallFunction` behaves differently — it's because the **JIT-enabled build takes an additional code path** (parameter profiling) that the interpreter-only build never executes.

**What `RecordParameterInfo` does:**
1.  Reads `inParams[index]` (a `Var` pointer from the argument array).
2.  Calls `ValueType::Merge(var)` → `ValueType::FromObject(recyclableObject)`.
3.  `FromObject` does `recyclableObject->GetTypeId()` — dereferences the object's `Type*` pointer.
4.  If the `Var` is corrupt, uninitialized, or not a real object, this dereference is a **SIGSEGV**.

### Possible Root Causes (Revised)

1.  **Bootstrap ordering / half-initialized objects:** During `InitializeTypedArrayPrototype`, some objects may not be fully constructed. The profiling code inspects them *before* they're ready. In `chinta64`, profiling never runs, so this state is never observed. This would be a latent bug in ChakraCore's bootstrap sequence, exposed only when JIT profiling is enabled.

2.  **`argCount` vs `callInfo.Count` mismatch:** The C++ call site in `JavascriptFunction.cpp:1449` passes `argCount` (computed via `GetArgCountWithExtraArgs()`) as the third parameter to `arm64_CallFunction`, and `args.Info` (containing `Count`) as the second. If these disagree, `arm64_CallFunction` would copy the wrong number of values, but `callInfo.Count` would tell the callee to read more than were actually placed — reading garbage from the frame.

3.  **The Apple-specific `arm64_CallJIT` dispatch:** `JavascriptFunction.cpp:1440` has:
    ```cpp
    if (argCount > 6 && VarTo<JavascriptFunction>(function)->IsScriptFunction())
        arm64_CallJIT(...)   // overflow-only layout
    else
        arm64_CallFunction(...)  // contiguous frame layout
    ```
    If a function with `argCount <= 6` goes through `arm64_CallFunction` but later gets called with `argCount > 6` via `arm64_CallJIT`, or if the `IsScriptFunction()` check incorrectly routes a native function through `arm64_CallJIT`, the stack layout would be wrong.

### Recommended Next Steps (Revised)

1.  **Quick experiment: Disable profiling.** Force `ShouldDoProfile()` to return `false` in the JIT build (e.g., set `ExecutionMode` to `FullInterpreter`). If `min_ta.js` passes, the crash is confirmed to be in the profiling path — not in `arm64_CallFunction` at all.

2.  **Diagnostic: Print the bad `Var`.** Add a temporary `fprintf` in `RecordParameterInfo` (or `ValueType::Merge`) to print the pointer value before dereferencing. A recognizable pattern (code address, stack address, NULL) would immediately narrow the source of corruption.

3.  **Check `argCount` vs `callInfo.Count`.** Add an assertion at the top of `arm64_CallFunction` (or in the C++ call site) that `argCount == callInfo.Count`. A mismatch would explain frame corruption without the LDP being at fault.


