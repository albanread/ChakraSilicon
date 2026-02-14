# Bug Report: ARM64 JIT CallDirect to Variadic C++ Helpers Fails on DarwinPCS

**Severity:** High — crashes and TypeErrors on all JIT-inlined builtin method calls
**Platform:** Apple Silicon (ARM64 / AArch64, DarwinPCS)
**Component:** JIT Backend — `LowerMD.cpp` (Lowerer), `Inline.cpp`, `Lower.cpp`
**Status:** In progress (targeted fix landed; full validation pending)
**Reproducible:** Yes — deterministic after JIT tier-up
**Related:** [ARM64_JIT_ARGUMENT_CORRUPTION.md](ARM64_JIT_ARGUMENT_CORRUPTION.md) — both bugs share the same root cause (DarwinPCS register/stack ABI mismatch)

---

## 2026-02-13 Update

Current code now uses targeted DarwinPCS stack shadowing for `CallDirect` paths (instead of unconditional shadowing for all calls), while preserving JS overflow-argument layout for `CallI`.

The companion 7+ argument corruption bug is now validated fixed. This varargs report remains open until the dedicated varargs repro suite is re-run end-to-end on the updated build.

---

## Summary

When the ARM64 JIT compiles a JavaScript function that calls builtin methods (e.g., `Array.indexOf`, `String.indexOf`, `TypedArray.fill`, `Array.includes`) in a hot loop, the function crashes with either a `TypeError` or a segmentation fault after JIT compilation. This happens because the JIT emits `CallDirect` instructions that call **variadic C++ entry points** (e.g., `JavascriptArray::EntryIndexOf(RecyclableObject*, CallInfo, ...)`) using the standard register-based ARM64 calling convention, but on Apple's DarwinPCS, variadic arguments after the last named parameter must be passed **on the stack**, not in registers. The `ARGUMENTS` / `DECLARE_ARGS_VARARRAY` macro uses `va_start` to extract arguments from the stack, finding garbage instead of the actual arguments.

---

## Root Cause

### The DarwinPCS variadic calling convention

On Apple ARM64 (DarwinPCS), variadic functions have a special calling convention: all variadic arguments (those after `...`) are passed on the stack, never in registers. This differs from standard AAPCS64 (Linux ARM64) and Windows ARM64, where variadic arguments may be passed in registers.

### The CallDirect code path

When the JIT decides to inline a builtin method call (e.g., `arr.indexOf(42)`), it goes through this path:

1. **`InliningDecider.cpp`** recognizes `indexOf` as `JavascriptArray_IndexOf` and sets `inlineCandidateOpCode = CallDirect`
2. **`Inline.cpp:SetupInlineInstrForCallDirect`** binds it to `HelperArray_IndexOf`
3. **`JnHelperMethodList.h`** maps `HelperArray_IndexOf` → `JavascriptArray::EntryIndexOf`
4. **`Lower.cpp:LowerCallDirect`** → `GenerateDirectCall` → `m_lowererMD.LowerCallArgs`
5. **`arm64/LowerMD.cpp:LowerCallArgs`** places arguments in registers `x0`–`x7` per the standard (non-variadic) ARM64 convention

### The C++ entry point signature

All affected entry points have the same variadic signature:

```cpp
Var JavascriptArray::EntryIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
Var JavascriptString::EntryIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
Var JavascriptArray::EntryIncludes(RecyclableObject* function, CallInfo callInfo, ...)
Var TypedArrayBase::EntryIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
// ... and all other HELPERCALLCHK-registered Entry* functions
```

Named parameters: `function` (x0) and `callInfo` (x1) — these work fine.
Variadic parameters: `this`, user args — DarwinPCS requires these **on the stack**.

### The argument extraction

These entry points use the `ARGUMENTS(args, callInfo)` macro, which expands to:

```cpp
// From Arguments.h — Apple ARM64 path:
#define DECLARE_ARGS_VARARRAY(va, ...)                              \
    va_list _vl;                                                    \
    va_start(_vl, callInfo);                                        \
    Js::Var* va = (Js::Var*)_vl + 2;
```

`va_start(_vl, callInfo)` on DarwinPCS points to the stack area where the compiler expects variadic args to have been placed. Since the JIT put them in registers (`x2`–`x7`) instead, `va_list` points to garbage on the stack.

### The collision

The `args[0]` (the `this` pointer) retrieved via `va_list` is garbage. When the runtime checks whether `this` is a TypedArray (`VarIs<TypedArrayBase>(args[0])`), it fails, producing:

- **`TypeError: 'this' is not a typed array object`** — for TypedArray methods
- **Segmentation fault** — for String/Array methods where the garbage pointer is dereferenced

### Connection to the argument corruption bug

The previous bug ([ARM64_JIT_ARGUMENT_CORRUPTION.md](ARM64_JIT_ARGUMENT_CORRUPTION.md)) had DarwinPCS shadow stores that wrote **all** register arguments to the stack on **every** JIT call. Those shadow stores were removed because they collided with overflow argument slots for JS-to-JS calls with 6+ parameters. However, they were also inadvertently providing the stack-based argument copies needed by `CallDirect` targets. Removing them fixed the JS-to-JS corruption but broke `CallDirect` to variadic C++ helpers.

**Both bugs stem from the same fundamental problem: the JIT has a single `LowerCallArgs` code path that is used for two fundamentally different call types with different ABI requirements on DarwinPCS.**

---

## Symptoms

1. **TypeErrors on TypedArray methods**: `TypeError: 'this' is not a typed array object` when calling `TypedArray.indexOf()`, `.fill()`, `.includes()` etc. in a hot loop after JIT compilation.

2. **Segmentation faults on String/Array methods**: `String.indexOf()`, `Array.indexOf()` etc. crash with SIGSEGV when the garbage `this` pointer is dereferenced.

3. **Non-deterministic iteration threshold**: The failure occurs after the enclosing function is JIT-compiled (typically iteration 3–85 depending on the method and loop structure).

4. **Works in interpreter**: All tests pass with `DISABLE_JIT=ON`.

5. **Works with low iteration counts**: Single calls or short loops work because the function hasn't been JIT-compiled yet — the interpreter path uses `arm64_CallFunction.S` which builds the correct contiguous stack frame.

---

## Reproduction

### Minimal reproducer

```javascript
function test() {
  var arr = new Int32Array(4);
  arr[0] = 42;
  var r = -1;
  for (var j = 0; j < 10; j++) {
    r = arr.indexOf(42);
  }
  return r;
}

for (var i = 0; i < 100; i++) {
  try {
    var r = test();
    if (r !== 0) { print("FAIL iter " + i + ": got " + r); break; }
  } catch(e) {
    print("FAIL iter " + i + ": " + e);
    break;
  }
}
print("done");
```

**Expected:** `done`
**Actual:** `FAIL iter 85: TypeError: 'this' is not a typed array object`

### Broader test showing all affected method types

```javascript
// TypedArray.fill - TypeError
function testFill() {
  var arr = new Int32Array(16);
  for (var i = 0; i < 200; i++) { arr.fill(0); }
}

// TypedArray.includes - TypeError
function testIncludes() {
  var arr = new Int32Array(16);
  arr[0] = 42;
  for (var i = 0; i < 200; i++) { arr.includes(42); }
}

// String.indexOf - Segfault
function testStringIndexOf() {
  var s = "hello world";
  for (var i = 0; i < 50; i++) { s.indexOf("world"); }
}

// Float32Array.indexOf - TypeError
function testFloat32() {
  var arr = new Float32Array(16);
  arr[0] = 3.14;
  for (var i = 0; i < 200; i++) { arr.indexOf(3.14); }
}
```

All fail after JIT compilation of the enclosing function.

---

## Affected Code Paths

| File | Lines | Description |
|------|-------|-------------|
| `lib/Backend/arm64/LowerMD.cpp` | 646–720 | `LowerCallArgs` — places arguments in registers for ALL calls, including `CallDirect` to variadic helpers |
| `lib/Backend/Lower.cpp` | 12308–12313 | `GenerateDirectCall` — calls `LowerCallArgs` without distinguishing variadic targets |
| `lib/Backend/Inline.cpp` | 3652–3730 | `SetupInlineInstrForCallDirect` — binds builtin IDs to `Entry*` helpers |
| `lib/Backend/JnHelperMethodList.h` | 478–497 | `HELPERCALLCHK` definitions for all affected `Entry*` helpers |
| `lib/Runtime/Language/Arguments.h` | 33–38 | `DECLARE_ARGS_VARARRAY` — Apple ARM64 path uses `va_start` to extract args from stack |
| `lib/Runtime/Library/JavascriptArray.cpp` | 4038 | `EntryIndexOf` — variadic `(... )` signature |
| `lib/Runtime/Library/TypedArray.cpp` | 2643–2657 | `ValidateTypedArray` — throws `JSERR_This_NeedTypedArray` when `args[0]` is garbage |

### Full list of affected `HELPERCALLCHK`-registered Entry functions

All of these are called via `CallDirect` from JIT and use variadic `...` signatures:

| Helper | C++ Target |
|--------|-----------|
| `Array_At` | `JavascriptArray::EntryAt` |
| `Array_Concat` | `JavascriptArray::EntryConcat` |
| `Array_IndexOf` | `JavascriptArray::EntryIndexOf` |
| `Array_Includes` | `JavascriptArray::EntryIncludes` |
| `Array_Join` | `JavascriptArray::EntryJoin` |
| `Array_LastIndexOf` | `JavascriptArray::EntryLastIndexOf` |
| `Array_Reverse` | `JavascriptArray::EntryReverse` |
| `Array_Shift` | `JavascriptArray::EntryShift` |
| `Array_Slice` | `JavascriptArray::EntrySlice` |
| `Array_Splice` | `JavascriptArray::EntrySplice` |
| `Array_Unshift` | `JavascriptArray::EntryUnshift` |
| `Array_IsArray` | `JavascriptArray::EntryIsArray` |
| `String_*` helpers | Various `JavascriptString::Entry*` |
| `RegExp_*` helpers | Various `JavascriptRegExp::Entry*` |

---

## Proposed Fix

### Approach: Selective shadow stores for CallDirect targets only

The fix must emit DarwinPCS shadow stores (register args → stack) **only** for `CallDirect` calls to variadic C++ helpers, while keeping overflow-only stack layout for JS-to-JS `CallI` calls.

### Option A: Gate shadow stores on call opcode

In `LowerCallArgs`, check whether the call instruction's opcode is `CallDirect`. If so, emit the DarwinPCS shadow stores. If it's a JS `CallI`, do not:

```cpp
// In LowerCallArgs, after placing arg in register:
if (opndParam->IsRegOpnd())
{
    callInstr->InsertBefore(argInstr);

#if defined(__APPLE__)
    // DarwinPCS: Only emit shadow stores for CallDirect to C++ variadic helpers.
    // JS-to-JS calls (CallI) must NOT have shadow stores because they collide
    // with overflow argument slots (see ARM64_JIT_ARGUMENT_CORRUPTION.md).
    if (callInstr->m_opcode == Js::OpCode::CallDirect)
    {
        Js::ArgSlot stackSlot = argSlotNum + extraParams;
        IntConstType offset = stackSlot * MachRegInt;
        IR::RegOpnd * spBase = IR::RegOpnd::New(nullptr, this->GetRegStackPointer(), TyMachReg, this->m_func);
        IR::IndirOpnd * stackOpnd = IR::IndirOpnd::New(spBase, int32(offset), TyMachReg, this->m_func);
        IR::Instr * storeInstr = IR::Instr::New(Js::OpCode::STR, stackOpnd, opndParam->AsRegOpnd(), this->m_func);
        callInstr->InsertBefore(storeInstr);

        if (this->m_func->m_argSlotsForFunctionsCalled < (uint32)(stackSlot + 1))
        {
            this->m_func->m_argSlotsForFunctionsCalled = stackSlot + 1;
        }
    }
#endif
}
```

Similarly for the function object (slot 0) and callInfo (slot 1) shadow stores in `LowerCallI`.

### Option B: Non-variadic wrappers for all Entry* functions

Create non-variadic wrapper functions for each `Entry*` function that accept a fixed argument list and extract `this` and user arguments explicitly, then forward to the original. This eliminates the `va_list` dependency entirely.

**Pros:** Cleaner long-term, no reliance on DarwinPCS shadow stores.
**Cons:** Requires creating ~30+ wrapper functions and updating `JnHelperMethodList.h`.

### Option C: Change DECLARE_ARGS_VARARRAY for JIT CallDirect

Create an alternative argument extraction path that reads from registers (via the callee's homed copy) instead of `va_list` when the caller is JIT code. This would require flags or a different entry point signature.

### Recommended approach

**Option A** is the simplest and safest immediate fix. It can be combined with Option B as a cleaner long-term solution later. The key insight is that the call opcode already distinguishes the two cases — `CallDirect` = C++ variadic helper, `CallI` = JS function.

The previous argument corruption fix correctly removed shadow stores for JS `CallI`. This fix adds them back selectively for `CallDirect` only.

### Implementation details

Three locations in `LowerMD.cpp` need conditional shadow stores:

1. **Register args in `LowerCallArgs`** (loop body, ~line 686): shadow-store register args only for `CallDirect`
2. **Function object in `LowerCallI`** (~line 608): shadow-store function object only for `CallDirect` — BUT `LowerCallI` is only called for JS `CallI`, never for `CallDirect`. The function object for `CallDirect` is loaded via `LoadHelperArgument` in `GenerateDirectCall`, so this location doesn't need to change.
3. **CallInfo in `LowerCallArgs`** (~line 716): shadow-store callInfo only for `CallDirect`

### Checking the opcode

At the point where `LowerCallArgs` runs, the call instruction opcode may have already been changed by lowering. The correct check depends on what opcode `CallDirect` has at this point. The instruction flow is:

- `LowerCallDirect` changes `callInstr` opcode via `GenerateDirectCall` → `LowerCall`
- `LowerCallArgs` is called inside `GenerateDirectCall` **before** `LowerCall`
- At this point, `callInstr->m_opcode` should still be `Js::OpCode::CallDirect`

This needs verification during implementation.

---

## Impact & Risk Assessment

- **Correctness:** ALL JIT-inlined builtin method calls crash or produce TypeErrors on Apple Silicon. This blocks any meaningful JIT testing.
- **Scope:** Affects every JavaScript program that calls builtin methods in hot loops (which is essentially all real-world code).
- **Interaction with Bug B fix:** The two fixes must be applied together. Removing shadow stores globally (Bug B fix) without adding them back for `CallDirect` (this fix) leaves JIT broken for builtins.
- **Windows ARM64:** Not affected — Windows AAPCS64 passes variadic args in registers.

---

## Environment

- **Architecture:** Apple Silicon (M1/M2/M3), ARM64 / AArch64
- **ABI:** DarwinPCS (Apple's ARM64 calling convention)
- **Build:** Release `ch` binary, JIT-enabled ARM64 target (`chjita64`)
- **Compiler:** Apple Clang (Xcode toolchain)
- **Not affected:** Windows ARM64, x86/x64 builds, interpreter-only builds
