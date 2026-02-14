# Bug Report: ARM64 JIT Argument Corruption on Apple Silicon (DarwinPCS)

**Severity:** High — silent data corruption at runtime
**Platform:** Apple Silicon (ARM64 / AArch64, DarwinPCS)
**Component:** JIT Backend — `LowerMD.cpp` (Lowerer) + `arm64_CallFunction.S` (call trampoline)
**Status:** Fixed (validated on current branch/build)
**Reproducible:** Not reproduced in current validated repro tests

---

## 2026-02-13 Validation Update

Validated with current `build/chjita64/bin/ch/ch`:

- `tests/repro_cases/test_arg_corruption.js` → `PASS: 7-param test`
- `tests/repro_cases/test_7p_900.js` → correct `28` results through iteration 899, then `DONE`

The 7+ argument corruption/hang symptom described in this report was not reproduced after the fix set now on `main`.

---

## Summary

When the ARM64 JIT compiles a JavaScript function that accepts **6 or more named parameters** (8+ total argument slots including `function` and `callInfo`), the first overflow argument is silently replaced by the **function object pointer**. This is caused by a stack-slot collision between the Apple-specific DarwinPCS "shadow store" for the function object and the standard overflow-argument slot, both of which target `[SP+0]` in the outgoing call frame.

The bug is a **pre-existing ARM64 calling-convention mismatch** and is **not** caused by NEON/vectorization work. It affects any JIT-compiled code path with enough arguments to spill to the stack.

---

## Root Cause

### The collision in one sentence

The DarwinPCS shadow store writes the function object to `[SP+0]` in the outgoing stack area, which is the **same location** where `GetOpndForArgSlot` places the 9th argument (first overflow slot), clobbering it.

### Detailed analysis

The Chakra ARM64 JIT calling convention uses 8 integer registers (`x0`–`x7`) for the first 8 argument slots:

| Slot | Contents     | Register |
|------|-------------|----------|
| 0    | function    | x0       |
| 1    | callInfo    | x1       |
| 2    | values[0] ("this") | x2 |
| 3    | values[1] (param 1) | x3 |
| …    | …           | …        |
| 7    | values[5] (param 5) | x7 |
| 8    | values[6] (param 6) | **stack [SP+0]** |
| 9    | values[7] (param 7) | **stack [SP+8]** |

`NUM_INT_ARG_REGS` is 8 (defined in `lib/Backend/arm64/EncoderMD.h:31–32`). When a slot index ≥ 8, `GetOpndForArgSlot` subtracts 8 and computes a stack offset:

```ChakraSilicon/ChakraCore/lib/Backend/arm64/LowerMD.cpp#L827-835
        {
            // Create a stack slot reference and bump up the size of this function's outgoing param area,
            // if necessary.
            argSlot = argSlot - NUM_INT_ARG_REGS;
            IntConstType offset = argSlot * MachRegInt;
            IR::RegOpnd * spBase = IR::RegOpnd::New(nullptr, this->GetRegStackPointer(), TyMachReg, this->m_func);
            opndParam = IR::IndirOpnd::New(spBase, int32(offset), type, this->m_func);

            if (this->m_func->m_argSlotsForFunctionsCalled < (uint32)(argSlot + 1))
```

So slot 8 → offset `(8-8) * 8 = 0` → **`[SP+0]`**.

Meanwhile, `LowerCallI` contains Apple-specific code that shadow-stores the function object to `[SP+0]` for DarwinPCS `va_list` compatibility:

```ChakraSilicon/ChakraCore/lib/Backend/arm64/LowerMD.cpp#L608-621
#if defined(__APPLE__)
    // DarwinPCS: also store function object (slot 0) to outgoing stack area
    // so that variadic callees can find it via va_list.
    {
        IntConstType offset = 0 * MachRegInt;
        IR::RegOpnd * spBase = IR::RegOpnd::New(nullptr, this->GetRegStackPointer(), TyMachReg, this->m_func);
        IR::IndirOpnd * stackOpnd = IR::IndirOpnd::New(spBase, int32(offset), TyMachReg, this->m_func);
        Lowerer::InsertMove(stackOpnd, opndParam, callInstr);

        if (this->m_func->m_argSlotsForFunctionsCalled < 1)
        {
            this->m_func->m_argSlotsForFunctionsCalled = 1;
        }
    }
#endif
```

Both writes target **`[SP+0]`**. The IR instruction ordering is:

1. `LowerCallArgs` processes user arguments — the 6th named param (slot 8) is stored to `[SP+0]`
2. `LowerCallI` stores the function object into `x0` (slot 0)
3. `LowerCallI` shadow-stores the function object to `[SP+0]` ← **clobbers step 1**
4. `BLR` to the callee

The callee then reads `[SP+0]` expecting the 6th parameter but finds the function object instead.

### Secondary issue: `arm64_CallFunction.S` frame layout mismatch

The Apple `arm64_CallFunction.S` trampoline also differs from the Windows `arm64_CallFunction.asm` in a way that compounds the problem:

**Windows** (`arm64_CallFunction.asm`) — puts **only** overflow args (`values[6+]`) on the stack, then loads `x2`–`x7` directly from the `values[]` array:

```ChakraSilicon/ChakraCore/lib/Runtime/Library/arm64/arm64_CallFunction.asm#L64-L78
StackAlloc
    add     x15, x5, #1                         ; round (param_count - 6) up by 1
    lsr     x15, x15, #1                        ; divide by 2
    bl      __chkstk                            ; ensure stack is allocated
    sub     sp, sp, x15, lsl #4                 ; then allocate the space
    add     x3, x3, #48                         ; use x3 = source
    mov     x4, sp                              ; use x4 = dest
CopyLoop
    subs    x5, x5, #1                          ; decrement param count by 1
    ldr     x7, [x3], #8                        ; read param from source
    str     x7, [x4], #8                        ; store param to dest
    bne     CopyLoop                            ; loop until all copied
    b       CopyAll                             ; jump ahead to copy all 6 remaining parameters
```

This produces the layout the JIT callee expects: `[SP+0]` = `values[6]`, `[SP+8]` = `values[7]`, etc.

**Apple** (`arm64_CallFunction.S`) — copies **all** args (including `function`, `callInfo`, `values[0..5]`) into a contiguous stack frame, then loads `x2`–`x7` from that frame:

```ChakraSilicon/ChakraCore/lib/Runtime/Library/arm64/arm64_CallFunction.S#L34-L57
    mov     x9, x4                              ; save entryPoint in scratch register x9
    add     x5, x2, #3                          ; add 3 to param count (function + callInfo + round-up)
    lsr     x5, x5, #1                          ; divide by 2 (for 16-byte alignment)
    sub     sp, sp, x5, lsl #4                  ; then allocate the space
    mov     x6, sp                              ; use x6 = dest
    str     x0, [x6], 8                         ; store function pointer
    str     x1, [x6], 8                         ; store info pointer

    cmp     x2, #0                              ; check for 0 params
    beq     LOCAL_LABEL(LoadRegs)

    LOCAL_LABEL(CopyLoop):
    subs    x2, x2, #1                          ; decrement param count by 1
    ldr     x7, [x3], #8                        ; read param from source
    str     x7, [x6], #8                        ; store param to dest
    bne     LOCAL_LABEL(CopyLoop)               ; loop until all copied

    LOCAL_LABEL(LoadRegs):
    ; Load values[0..5] into x2-x7 from the stack frame we just built.
    ; The stack at sp is: [function, callInfo, values[0], values[1], ...]
    ldp     x2, x3, [sp, #0x10]                ; x2 = values[0], x3 = values[1]
    ldp     x4, x5, [sp, #0x20]                ; x4 = values[2], x5 = values[3]
    ldp     x6, x7, [sp, #0x30]                ; x6 = values[4], x7 = values[5]
```

This produces a stack layout where:
- `[SP+0x00]` = function object
- `[SP+0x08]` = callInfo
- `[SP+0x10]` = values[0]
- …
- `[SP+0x38]` = values[5]
- `[SP+0x40]` = values[6] ← **actual first overflow arg**

But the JIT callee reads the first overflow arg from `[FP-relative]` which maps to `[caller_SP+0]` — finding the function object instead of `values[6]`.

### Both issues reinforce the same symptom

Whether the call originates from JIT-to-JIT (shadow-store collision in `LowerCallArgs`/`LowerCallI`) or from the interpreter trampoline (`arm64_CallFunction.S` frame layout), the callee reads the **function object** where it expects the **first overflow argument**.

---

## Symptoms

1. **Silent argument corruption**: The 6th named JavaScript parameter (the first value that spills to the stack) is silently replaced by the function object. Stringifying it yields the function's source text.

2. **Runtime TypeErrors in polymorphic code**: When combined with bailout/recompile cycles on polymorphic TypedArray code, the corrupted argument values can trigger cascading TypeError failures when the JIT recompiles and attempts to use the corrupted value.

3. **No crash or assertion failure**: The corruption is completely silent in Release builds. The program continues with incorrect values.

---

## Reproduction

### Minimal reproducer: `tests/neon_benchmarks/jit_bugb_only.js`

```ChakraSilicon/tests/neon_benchmarks/jit_bugb_only.js#L12-L30
function sixParams(a1, a2, a3, a4, a5, a6) {
  var arr = new Int32Array(64);
  arr.fill(0);
  arr[63] = -999;

  var result = -1;
  for (var iter = 0; iter < 200; iter++) {
    result = arr.indexOf(-999);
  }

  // Check if a6 got corrupted to the function object
  var s = "" + a6;
  if (s.length > 50 || s.indexOf("function") === 0) {
    return "CORRUPTED:" + s.substring(0, 40);
  }
  return "OK:result=" + result + ",a6=" + a6;
}
```

**Run:**

```/dev/null/shell.sh#L1-3
ch jit_bugb_only.js
# or with backend dumps:
ch -Dump:BackEnd jit_bugb_only.js 2>&1 | tee bugb_dump.txt
```

**Expected output:**

```/dev/null/expected.txt#L1-5
PASS post-JIT iter 0: OK:result=63,a6=post-0
PASS post-JIT iter 1: OK:result=63,a6=post-1
PASS post-JIT iter 2: OK:result=63,a6=post-2
PASS post-JIT iter 3: OK:result=63,a6=post-3
PASS post-JIT iter 4: OK:result=63,a6=post-4
```

**Actual output:**

```/dev/null/actual.txt#L1
FAIL at warmup iter N: CORRUPTED:function sixParams(a1, a2, a3, a4
```

The `a6` parameter becomes the string representation of the `sixParams` function object.

### Extended reproducer: `tests/neon_benchmarks/jit_minimal_repro.js`

This file also covers Bug A (polymorphic TypedArray TypeError), which is a related but distinct manifestation of the same underlying ABI mismatch interacting with bailout/recompile.

---

## Affected Code Paths

| File | Lines | Description |
|------|-------|-------------|
| `lib/Backend/arm64/LowerMD.cpp` | 608–621 | DarwinPCS shadow store of function object to `[SP+0]` |
| `lib/Backend/arm64/LowerMD.cpp` | 702–722 | DarwinPCS shadow store of register args to outgoing stack |
| `lib/Backend/arm64/LowerMD.cpp` | 754–768 | DarwinPCS shadow store of callInfo to outgoing stack |
| `lib/Backend/arm64/LowerMD.cpp` | 827–835 | `GetOpndForArgSlot` overflow computation (slot−8 → `[SP+0]`) |
| `lib/Runtime/Library/arm64/arm64_CallFunction.S` | 34–57 | Apple trampoline: contiguous frame with function at `[SP+0]` |
| `lib/Backend/arm64/EncoderMD.h` | 29–32 | `NUM_INT_ARG_REGS = 8` |

---

## Proposed Fix

Two changes are required (both should be applied):

### Fix A: Remove unconditional DarwinPCS shadow stores in `LowerCallI` / `LowerCallArgs`

The shadow stores at lines 608–621, 702–722, and 754–768 of `LowerMD.cpp` unconditionally write all register arguments into the outgoing stack area for **every** JS call. This is only needed for calls to C++ variadic callees (e.g., `ExternalFunctionThunk`) that use `va_list` to walk arguments on the stack.

**Action:** Remove or gate these shadow stores so they only apply when the call target is known to be a C-variadic external thunk. For JS-to-JS and JS-to-JIT calls, the shadow stores must not be emitted because they collide with overflow argument slots.

**Minimal safe patch:**

1. Delete the `#if defined(__APPLE__)` shadow-store blocks at lines 608–621, 702–722, and 754–768.
2. If `ExternalFunctionThunk` or other C-variadic callees break, add a targeted code path that only emits shadow stores when the callee is identified as requiring `va_list`-style argument passing (this can be detected by checking whether the call target is an external/native function).

### Fix B: Update `arm64_CallFunction.S` to match Windows overflow-only layout

The Apple trampoline should only place **overflow arguments** (`values[6+]`) on the stack, matching the layout that the JIT callee expects. Register-bound arguments (`values[0..5]`) should be loaded directly from the `values[]` array into `x2`–`x7`, without first copying them to the stack.

**Action:** Rewrite the Apple `arm64_CallFunction.S` to follow the same logic as the Windows `arm64_CallFunction.asm`:

1. Check if `argCount > 6`. If so, allocate stack space for `argCount - 6` overflow slots only.
2. Copy `values[6..N-1]` to `[SP+0], [SP+8], …`
3. Load `x2`–`x7` from `values[0..5]` directly (using the `values` pointer in `x3`/`x9`).
4. `BLR` to entry point.

If the contiguous-frame layout is needed for the **stack walker**, a separate mechanism (e.g., metadata or a debug-only frame) should be used that does not interfere with the callee's argument reads.

---

---

## Design Approach: "One Protocol"

### The Core Principle

The fundamental flaw is an **ABI confusion**: the macOS implementation attempts to apply C-style variadic argument rules (DarwinPCS shadow stores) to internal JavaScript-to-JavaScript calls, which directly conflicts with the JIT's optimized register allocation strategy. DarwinPCS is an **external boundary constraint** — it governs how Chakra interacts with the OS and C++ runtime — but it must not be allowed to dictate the internal JIT calling convention.

**Design rule:** Enforce a single, canonical stack frame layout for JIT-compiled JavaScript functions on all ARM64 platforms (Windows, Linux, macOS):

- **Registers `x0`–`x7`:** Reserved for Arguments 0–7.
- **Stack `[SP+0]` onward:** Reserved *strictly* for overflow arguments (Argument 8+).
- **Platform independence:** The JIT does not care which OS it is running on when calling another JS function.

### Visualization of the Collision

The current design creates a "collision zone" at `[SP+0]`:

**Current (buggy) Apple layout — caller's outgoing frame:**

```/dev/null/layout_buggy.txt#L1-8
      [ Caller's Stack Frame ]
      | ...                  |
SP -> +----------------------+ <=== COLLISION POINT
      | Shadow: Function Obj | <--- Written by LowerMD (DarwinPCS shadow store)
      +----------------------+
      | Shadow: CallInfo     | <--- Written by LowerMD (DarwinPCS shadow store)
      +----------------------+
      | Shadow: values[0]... | <--- More shadow stores for x2-x7
```

**JIT callee's expectation (Windows / standard / correct):**

```/dev/null/layout_correct.txt#L1-7
      [ Caller's Stack Frame ]
      | ...                  |
SP -> +----------------------+
      | Overflow Arg 1 (a6)  | <--- Read by callee via GetOpndForArgSlot(8)
      +----------------------+
      | Overflow Arg 2 (a7)  | <--- Read by callee via GetOpndForArgSlot(9)
      +----------------------+
```

Because the JIT callee reads `[SP+0]` expecting the 6th named parameter, but `LowerMD` wrote the function object pointer there, the application interprets a pointer as data — producing silent corruption.

### Step-by-Step Implementation Plan

#### Step A: Align the Trampoline (Entry Point)

`arm64_CallFunction.S` is the adapter that transitions from C++ interpreter dispatch into JIT-compiled JS. The Apple version currently creates a "fat" contiguous stack frame containing all arguments. It must be rewritten to match the Windows `arm64_CallFunction.asm` logic: **overflow-only**.

**Action:** Modify `arm64_CallFunction.S`:

- If `argCount <= 6`: Allocate 0 stack space. Load `values[0..5]` into `x2`–`x7` directly from the `values[]` array pointer.
- If `argCount > 6`: Allocate `(argCount - 6)` overflow slots (16-byte aligned). Copy only `values[6+]` to `[SP+0], [SP+8], …`. Then load `x2`–`x7` from `values[]`.

**Benefit:** When the JIT callee enters from C++, `[SP+0]` actually contains `values[6]`, matching the JIT's expectation.

#### Step B: Purge Internal Shadow Stores (The Lowerer)

`LowerMD.cpp` contains three `#if defined(__APPLE__)` blocks that indiscriminately dump register arguments to the outgoing stack area. This is defensive coding that has become destructive.

**Action:** Remove the unconditional shadow stores in `LowerCallI` (lines 608–621) and `LowerCallArgs` (lines 702–722, 754–768).

**Reasoning:** Internal JS functions are not C-variadic functions. They do not use `va_start` and do not look at the stack for registers `x0`–`x7`. Writing them to the stack is wasted CPU cycles and, in this case, **corrupts memory** by overwriting overflow argument slots.

#### Step C: Handle the C-Variadic Edge Case (Safe Interop)

If there is a legitimate need to call a C++ helper function that uses `va_list` (e.g., `ExternalFunctionThunk`), this must be handled explicitly at the ABI boundary — not applied globally to all calls.

**Action:** If `ExternalFunctionThunk` or other native interaction points break after Step B, introduce a specific call-target check (e.g., `CallFlags::IsVarArgC`) in the IR. Only emit DarwinPCS shadow stores when the call target is explicitly known to be a C-host function requiring variadic support. Do not apply this to standard JS-to-JS calls.

**Note:** `ExternalFunctionThunk` is called via the interpreter's `arm64_CallFunction.S` trampoline (which builds the contiguous frame for `va_list`), or via `CALL_ENTRYPOINT_NOASSERT` (which duplicates args to the stack). Neither path goes through JIT-emitted `CallI` instructions for external functions. Therefore, removing the shadow stores from the JIT lowerer is expected to be safe without any Step C work.

### Summary of Changes

| Component | Current State | Target Design |
|-----------|--------------|---------------|
| Trampoline (`arm64_CallFunction.S`) | Copies **all** args to stack (contiguous frame) | Copy **only overflow** args (`values[6+]`) to stack. Match Windows `.asm`. |
| JIT Lowerer (`LowerMD.cpp`) | Writes FuncObj/CallInfo/args to `[SP+0..56]` on every call | **Remove** shadow stores for JS calls. Overflow args own `[SP+0]`. |
| Stack Layout | Hybrid (shadow stores + overflow in same region) | Sparse (overflow only). One protocol, all platforms. |
| `DECLARE_ARGS_VARARRAY` | Relies on contiguous frame from trampoline | Unchanged — only used by C++ variadic entry points called via trampoline. |

### Risk Mitigation

- **Stack walking:** Apple's frame pointer (`x29`) chains are sufficient for unwinding. The shadow stores are for argument access by `va_list`, not for unwinding. The JIT prologue homes `x0`–`x7` into the callee's frame, so the stack walker can find arguments there.
- **Clean build required:** Since this changes the internal calling convention, all object files must be recompiled. No stale `.o` files from the old layout can remain.
- **Regression tests:** `jit_bugb_only.js` and `jit_minimal_repro.js` must be added as gated regression tests for Apple Silicon CI.

---

## Suggested Patch Order

1. **Step 1 — Purge shadow stores (`LowerMD.cpp`):** Remove the three `#if defined(__APPLE__)` DarwinPCS shadow-store blocks. Re-run `jit_bugb_only.js` and the full test suite.
2. **Step 2 — Align trampoline (`arm64_CallFunction.S`):** Rewrite to overflow-only layout matching Windows `.asm`. Re-run tests.
3. **Step 3 — Update `DECLARE_ARGS_VARARRAY` if needed:** Verify `ExternalFunctionThunk` still works. If broken, add targeted interop handling (Step C above).
4. **Step 4 — Regression tests:** Add reproducers as gated tests for Apple Silicon CI. Clean build and full validation.

---

## Impact & Risk Assessment

- **Correctness:** Any JIT-compiled function with ≥ 6 named parameters produces silently incorrect results on Apple Silicon. This is not limited to NEON or TypedArray code — it affects all JS functions.
- **Scope:** The bug is latent in all Apple ARM64 builds with JIT enabled. Functions with ≤ 5 named parameters are unaffected (all 8 argument slots fit in registers).
- **NEON/vectorization:** This bug must be resolved before enabling JIT-side NEON vectorization, as vectorized code paths will use TypedArray helpers that may pass 6+ arguments and would produce silently incorrect vector computations.
- **Windows ARM64:** Not affected. The Windows `arm64_CallFunction.asm` uses the correct overflow-only layout and has no DarwinPCS shadow-store code.

---

## Environment

- **Architecture:** Apple Silicon (M1/M2/M3), ARM64 / AArch64
- **ABI:** DarwinPCS (Apple's ARM64 calling convention)
- **Build:** Debug `ch` binary, Apple ARM64 target
- **Compiler:** Apple Clang (Xcode toolchain)
- **Not affected:** Windows ARM64, x86/x64 builds