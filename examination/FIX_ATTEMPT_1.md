# Fix Attempt #1: Unified ARM64 Thunk Prologue

**Date:** 2024 (Current Session)  
**Status:** FAILED - Still hangs  
**Approach:** Made Darwin ARM64 use same prologue as Windows ARM64

---

## The Hypothesis

Based on source code analysis, I hypothesized that the ARM64 Darwin thunk was broken because:

1. It only saved FP and LR (16 bytes)
2. It did NOT save X0-X7 to the stack
3. The ARGUMENTS macro expects parameters on the stack
4. Windows ARM64 saves X0-X7 and works fine
5. Therefore: Make Darwin match Windows

**Confidence Level:** 95% - seemed like a solid analysis

---

## What I Changed

### File: `ChakraCore/lib/Backend/InterpreterThunkEmitter.cpp`

#### Before (Darwin ARM64):
```asm
0xFD, 0x7B, 0xBF, 0xA9,   // stp fp, lr, [sp, #-16]!   ; Only 16 bytes
0xFD, 0x03, 0x00, 0x91,   // mov fp, sp
```

#### After (Unified for all ARM64):
```asm
0xFD, 0x7B, 0xBB, 0xA9,   // stp fp, lr, [sp, #-80]!   ; 80 bytes
0xFD, 0x03, 0x00, 0x91,   // mov fp, sp
0xE0, 0x07, 0x01, 0xA9,   // stp x0, x1, [sp, #16]     ; Save all registers
0xE2, 0x0F, 0x02, 0xA9,   // stp x2, x3, [sp, #32]
0xE4, 0x17, 0x03, 0xA9,   // stp x4, x5, [sp, #48]
0xE6, 0x1F, 0x04, 0xA9,   // stp x6, x7, [sp, #64]
```

### File: `ChakraCore/lib/Backend/InterpreterThunkEmitter.h`

Changed `InterpreterThunkSize` for Darwin ARM64 from 48 to 64 bytes.

### File: `scripts/build_target.sh`

Added CMake flags:
- `-DAPPLE_SILICON_JIT=ON`
- `-DPROHIBIT_STP_LDP=ON`

---

## The Result

**Test:** `./dist/chjita64/ch examination/test_jit_to_int.js`

**Output:**
```
=== JIT→INT Call Interface Test ===

Test 1: Math.max with fixed arguments
[HANGS - timeout after 5 seconds]
```

**Conclusion:** Still hangs on the first JIT→INT call. The fix did not work.

---

## Why It Failed: Analysis

### Theory #1: The Dynamic Thunk Does Something Different

The static thunk we modified just sets up the stack and jumps to the dynamic thunk via `blr x3`.

**Possibility:** The dynamic thunk on Darwin expects a DIFFERENT layout than on Windows, and we broke it by changing the static thunk without understanding what the dynamic thunk does.

**Evidence:** We never actually looked at what the dynamic thunk does. We just assumed it's the same across platforms.

### Theory #2: The CALL_ENTRYPOINT_NOASSERT Macro Matters

The macro for Darwin ARM64:
```cpp
#define CALL_ENTRYPOINT_NOASSERT(entryPoint, function, callInfo, ...) \
    entryPoint(function, callInfo, function, callInfo, ##__VA_ARGS__)
```

This duplicates `function` and `callInfo`. After our change:
- X0 = function (first)
- X1 = callInfo (first)
- X2 = function (duplicate)
- X3 = callInfo (duplicate)
- Saved to stack at [SP+16], [SP+24], [SP+32], [SP+40]

**But:** The macro comment says "varargs always passed via stack" for Darwin. Maybe the issue is that we're passing them in REGISTERS (X0-X7) when Darwin expects them ONLY on stack?

### Theory #3: We're Saving the Wrong Thing

When the thunk executes:
1. X0 = function object (from JIT)
2. X1 = callInfo (from JIT)
3. We save X0-X7 to [SP+16] through [SP+64]
4. Then we do: `ldr x2, [x0, #offset]` - loading from function object

**Problem:** After we save X0, we then OVERWRITE X0 and X2 with loads from memory!

```asm
stp x0, x1, [sp, #16]     ; Save X0, X1
stp x2, x3, [sp, #32]     ; Save X2, X3 (whatever they contain)
...
ldr x2, [x0, #0x00]       ; Load FunctionInfo (overwrites X2)
ldr x0, [x2, #0x00]       ; Load FunctionProxy (overwrites X0!)
ldr x3, [x0, #0x00]       ; Load DynamicInterpreterThunk
add x0, sp, #16           ; X0 = pointer to saved area
br  x3                    ; Jump
```

**So when we jump to the dynamic thunk:**
- X0 = SP + 16 (pointer to saved registers)
- X1 = callInfo (still original value)
- X2 = FunctionInfo (loaded value)
- X3 = dynamic thunk address (about to jump to)

**Stack at [SP+16]:**
- [SP+16] = original X0 (function object) ✓
- [SP+24] = original X1 (callInfo) ✓
- [SP+32] = original X2 (whatever was there - probably null or garbage)
- [SP+40] = original X3 (whatever was there - probably null or garbage)
- [SP+48] = original X4
- [SP+56] = original X5
- [SP+64] = original X6
- [SP+72] = original X7

### Theory #4: The `add x0, sp, #16` is Wrong

Windows thunk does the same thing:
```asm
add x0, sp, #16
br  x3
```

But maybe on Darwin, the dynamic thunk expects X0 to be the function object, NOT a pointer to the stack?

### Theory #5: We Need to Look at x64 Runtime Behavior

Our analysis was purely static (reading source code). We never actually RAN the x64 version under a debugger to see:
- What values are in registers at thunk entry
- What the dynamic thunk actually does
- What stack layout InterpreterHelper actually sees

**This was the original plan but we skipped it and went straight to implementing!**

---

## What We Should Have Done

1. **Debug the x64 version first** using the tools we prepared:
   - `examination/debug_x64_thunk.lldb`
   - Run under Rosetta with breakpoints
   - Capture actual register values
   - Disassemble dynamic thunk
   - See real stack layout

2. **Compare x64 disassembly with ARM64 source**

3. **Understand the dynamic thunk** before modifying static thunk

4. **Test incrementally** - maybe just add logging first, not change everything

---

## Next Steps

### Immediate: Actually Debug x64

Run the debugging session we prepared:
```bash
lldb ./dist/chjitx64/ch
(lldb) command source examination/debug_x64_thunk.lldb
(lldb) run examination/test_jit_to_int.js
```

Capture:
- Register values at thunk entry
- What dynamic thunk does
- Stack layout at InterpreterHelper
- How ARGUMENTS macro finds parameters

### Then: Compare ARM64 Runtime Behavior

Add extensive logging to ARM64 thunk:
- Log all register values on entry
- Log stack pointer
- Log what gets saved where
- See what's actually happening vs. what we think

### Alternative Approach: Look at the Macro More Carefully

The Darwin CALL_ENTRYPOINT_NOASSERT macro duplicates parameters. Maybe we need to adjust it INSTEAD of or IN ADDITION to the thunk?

Current macro:
```cpp
entryPoint(function, callInfo, function, callInfo, ##__VA_ARGS__)
```

What if we need:
```cpp
// Force all params to stack by passing MORE nulls in registers?
entryPoint(function, callInfo, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
           function, callInfo, ##__VA_ARGS__)
```

This would fill X0-X7 with (function, callInfo, null, null, null, null, null, null), and push the duplicates to stack.

---

## Lessons Learned

1. **Source code analysis is not enough** - runtime behavior can differ from expectations
2. **Test incrementally** - we changed too much at once
3. **Follow the original plan** - we had a good debugging strategy but skipped it
4. **Windows ≠ Darwin** - even on same architecture, OS differences matter
5. **The dynamic thunk is KEY** - we don't understand what it does

---

## Key Questions Still Unanswered

1. What does the dynamic thunk do on Darwin vs. Windows?
2. What does X0 contain when calling the dynamic thunk? Function object or stack pointer?
3. How does the ARGUMENTS macro actually compute addresses?
4. What does InterpreterHelper see when it enters?
5. Are we even reaching the dynamic thunk, or hanging in the static thunk?

---

## Status

**Back to research phase.** We need to understand, not guess.

The fix seemed logical based on source code, but runtime behavior proved otherwise. Time to actually debug and observe what's happening.

---

**Next File to Create:** `X64_ACTUAL_FINDINGS.md` - results from actual debugging session