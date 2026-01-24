# KEY INSIGHT: x64 vs ARM64 Call Chain and Calling Convention

## Critical Discovery from Debugging

After debugging the working x64 system, the key difference is **NOT** architectural complexity, but **calling convention**!

Both x64 and ARM64 have the same multi-level call chain, but they handle parameters differently.

## Complete Call Chain (Both Platforms)

```
JIT Code
  ↓
Static Generated Thunk (from InterpreterThunkEmitter.cpp)
  ↓
Dynamic Per-Function Thunk (generated at runtime per function)
  ↓
C++ InterpreterThunk function (InterpreterStackFrame.cpp)
  ↓
InterpreterHelper
  ↓
Built-in function
```

## x64 Architecture (WORKING)

### 1. Static Thunk (simple - just calls through)
```asm
call rax          ; RAX = points to next level
jmp  epilog
```

### 2. Passes JavascriptCallStackLayout structure
- All parameters on **STACK** (x64 calling convention)
- RDI = pointer to JavascriptCallStackLayout structure
- Structure contains: [function, callInfo, args...]

### 3. C++ InterpreterThunk receives:
```cpp
Var InterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...) {
    ARGUMENTS(args, callInfo);  // Macro reads from STACK
    // ...
}
```

## ARM64 Architecture (BROKEN)

### 1. Static Thunk (complex - does setup work)
```asm
stp fp, lr, [sp, #-80]!       ; Allocate stack
stp x0, x1, [sp, #16]         ; Save registers to stack
stp x2, x3, [sp, #32]
stp x4, x5, [sp, #48]
stp x6, x7, [sp, #64]
ldr x2, [x0, #offset]         ; Load FunctionInfo
ldr x0, [x2, #offset]         ; Load FunctionProxy  
ldr x3, [x0, #offset]         ; Load DynamicInterpreterThunk address
movz/movk x1, #addr           ; Load static thunk address
add x0, sp, #16               ; X0 = pointer to saved registers
br  x3                        ; Branch to dynamic thunk
```

### 2. Parameters in REGISTERS (ARM64 AAPCS64)
- X0-X7 contain parameters
- Varargs beyond 8 params go on stack
- ARGUMENTS macro expects args on STACK (not registers!)

### 3. C++ InterpreterThunk receives:
```cpp
Var InterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...) {
    ARGUMENTS(args, callInfo);  // Macro tries to read from STACK
    // But on ARM64 Darwin, args are in REGISTERS (X2-X7), not stack!
}
```

## The REAL Problem

**Calling convention mismatch between what the macro expects vs what ARM64 provides!**

### x64:
- Parameters always on stack → ARGUMENTS macro works ✓

### ARM64 Darwin:
- Parameters in registers (AAPCS64) → ARGUMENTS macro reads garbage ✗
- Static thunk saves registers to stack ✓
- But dynamic thunk may not preserve this layout? ✗
- Or X0 pointer is wrong? ✗

## The ARGUMENTS Macro Expectation

From `Arguments.h`:
```cpp
// At entry of JavascriptMethod the stack layout is:
//      [Return Address] [function] [callInfo] [arg0] [arg1] ...

Js::Var* pArgs = reinterpret_cast<Js::Var*>(addrOfReturnAddress) + 1;
```

The macro expects to find parameters **on the stack, right after return address**.

## Why Our Fix Failed

We saved X0-X7 to stack at [SP+16] through [SP+64], then set X0 = SP+16.

But:
1. The dynamic thunk may expect X0 to be the **function object**, not a pointer
2. OR the dynamic thunk doesn't exist properly on Darwin
3. OR the stack layout we created doesn't match what the dynamic thunk expects
4. OR the dynamic thunk doesn't properly call the C++ InterpreterThunk

## Next Steps

1. **Find and examine the dynamic thunk generation code**
   - Where is it generated?
   - What does it expect in X0?
   - Does it properly transition to C++ InterpreterThunk?

2. **Check if DYNAMIC_INTERPRETER_THUNK is enabled for Darwin ARM64**
   - Maybe it's disabled and falling back to something broken?

3. **Verify the dynamic thunk actually gets generated**
   - Add logging to GenerateDynamicInterpreterThunk()
   - Check if m_dynamicInterpreterThunk is nullptr

4. **Consider alternative: Make ARM64 simpler like x64**
   - Instead of jumping to dynamic thunk
   - Directly call C++ InterpreterThunk
   - Let it handle everything

## Status

We now understand the architecture. The issue is in how ARM64 bridges from registers to the stack-expecting ARGUMENTS macro, possibly through a missing or broken dynamic thunk.
