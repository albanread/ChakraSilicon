# ARM64 JIT Call Bugs — Status for Review

**Date:** 2026-02-13  
**Build:** `build/chjita64` (ARM64 JIT on Apple Silicon, DarwinPCS ABI)  
**Baseline:** All code changes REVERTED. LowerMD.cpp is clean/original.

---

## Two Distinct Bugs

### Bug A: Varargs (C++ Entry* functions crash)

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

**Fix — VALIDATED:**  
An assembly trampoline (`arm64_DebugTrampoline` in `arm64_CallFunction.S`) that:
1. Receives args in x0-x7 from JIT
2. Spills them to a contiguous stack frame: [function, callInfo, arg0, arg1, ...]
3. Calls the real C++ target via `blr x16`

When wired into LowerCall for helper calls, **TypedArray.indexOf passes.** This is the correct fix.

**Remaining problem:** Not all C++ Entry* calls go through the helper path. Some go through the indirect path (CallI / vtable dispatch). The trampoline cannot be blindly applied to indirect calls because those also carry JS→JS calls, and interposing a frame breaks JS→JS overflow arg access.

---

### Bug B: 7+ parameter JS→JS calls crash

**Symptom:** Any JS function with 7+ parameters segfaults when JIT-compiled.

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
| Trampoline on ALL calls (indirect + helper) | Fixed varargs, broke JS→JS calls | Trampoline interposes a frame; JS callee reads overflow from wrong offset |
| Trampoline on HELPER calls only | Fixed varargs for helpers; 7p still crashes | 7p crash is pre-existing Bug B, unrelated to trampoline |
| No changes (clean baseline) | 7p crashes, varargs crash | Both bugs exist in the original code |

---

## Infrastructure Built

All present in the codebase, can be wired in:

1. **`arm64_DebugTrampoline`** in `arm64_CallFunction.S`: Assembly trampoline that spills x0-x7 to a contiguous stack frame and calls target via x16. Also calls `debug_dump_jit_call` for diagnostics (controlled by env var).

2. **`arm64_DebugDump.cpp`**: C debug function. Captures call snapshots (regs, stack) into an in-memory ring buffer (4096 entries). Prints at exit via `atexit()` — zero I/O during hot path, does not disturb JIT timing.

3. **`arm64_CallDirectVarargs`** in `arm64_CallFunction.S`: Trampoline specifically for CallDirect varargs path.

4. **`Lower.cpp` (GenerateDirectCall)**: Modified to redirect CallDirect through `HelperCallDirectVarargs` on Apple ARM64.

5. **`JnHelperMethodList.h` / `JnHelperMethod.h`**: `HelperCallDirectVarargs` registered.

6. **`CMakeLists.txt`**: `arm64_DebugDump.cpp` added to build.

LowerMD.cpp `LowerCall` is currently **clean** — no trampoline, no shadow stores, no offset changes.

---

## What Needs To Happen

### For Bug A (varargs):
The trampoline approach works. Need to find where to insert it:
- **Helper calls:** Wire trampoline into LowerCall helper path. Already validated.
- **Indirect calls to Entry* functions:** Need a way to distinguish "indirect call to C++ Entry*" from "indirect call to JIT'd JS function" at the call site. Options:
  - Runtime check in trampoline (inspect target address to see if it's in JIT code range)
  - Always use trampoline + adjust JS callee to handle the extra frame
  - Tag indirect calls at lower time if the target is known to be a built-in

### For Bug B (7+ args):
Need to understand the actual bug. The callee reads overflow arg from `[CallerSP+8]`. The caller (when interpreted) puts it at the right place. When the caller is JIT'd — we've never seen the JIT'd caller code because tracing prevents JIT.

Key code to examine:
- `GetOpndForArgSlot()` for slot indices >= `NUM_INT_ARG_REGS` (8) — this determines where overflow args are written by the JIT caller
- `LowerEntryInstr()` and `ARM64StackLayout` — this determines where overflow args are read by the JIT callee (via `m_ArgumentsOffset`)
- The relationship between the caller's outgoing area and the callee's incoming frame

---

## Files of Interest

| File | Purpose |
|------|---------|
| `lib/Backend/arm64/LowerMD.cpp` | LowerCall, GetOpndForArgSlot, LowerEntryInstr, ARM64StackLayout |
| `lib/Backend/Lower.cpp` | GenerateDirectCall (CallDirect trampoline wiring) |
| `lib/Runtime/Library/arm64/arm64_CallFunction.S` | All assembly trampolines |
| `lib/Runtime/Library/arm64/arm64_DebugDump.cpp` | Ring buffer debug dump |
| `lib/Runtime/Language/Arguments.h` | DECLARE_ARGS_VARARRAY, CALL_ENTRYPOINT macros |
| `lib/Backend/JnHelperMethodList.h` | Helper method registrations |
