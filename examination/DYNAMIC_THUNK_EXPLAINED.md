# Dynamic Interpreter Thunk Explained

## Critical Finding

The "dynamic interpreter thunk" is NOT a separate piece of code. It's **an offset into the generated thunk block** created from the `InterpreterThunk[]` array in `InterpreterThunkEmitter.cpp`!

## How It Works

### Thunk Block Structure

```
[Page Boundary - Entry Point]
  HeaderSize bytes
  [InterpreterThunk code - static prologue/setup]
  
  ThunkSize bytes - Function 1's unique thunk
  ThunkSize bytes - Function 2's unique thunk  
  ThunkSize bytes - Function 3's unique thunk
  ...
  ThunkSize bytes - Function N's unique thunk
```

### GetNextThunk() Returns

1. **Entry point** (thunk): Points to start of page (the InterpreterThunk prologue)
2. **Dynamic thunk** (ppDynamicInterpreterThunk): Points to a unique offset per function

### Storage in FunctionBody

```cpp
FunctionBody::GenerateDynamicInterpreterThunk() {
    this->SetOriginalEntryPoint(
        scriptContext->GetNextDynamicInterpreterThunk(&this->m_dynamicInterpreterThunk)
    );
}
```

- `SetOriginalEntryPoint()` receives the **entry point** (start of page)
- `m_dynamicInterpreterThunk` stores the **unique offset** for this function

### When Function is Called

1. JIT code calls `function->GetOriginalEntryPoint()` → jumps to **entry point** (page start)
2. Entry point is the `InterpreterThunk[]` prologue code
3. Prologue loads `m_dynamicInterpreterThunk` from function object
4. Prologue jumps to that **dynamic offset**
5. Dynamic offset contains `Call[]` code: `blr x1; b epilog`
6. X1 contains address of... what? The C++ InterpreterThunk function!

## The ARM64 Code Flow

```asm
; Entry at page boundary (InterpreterThunk[] array)
stp fp, lr, [sp, #-80]!      ; Prologue
stp x0, x1, [sp, #16]         ; Save registers
...
ldr x2, [x0, #offset]         ; Load FunctionInfo from function object
ldr x0, [x2, #offset]         ; Load FunctionProxy
ldr x3, [x0, #offset]         ; Load m_dynamicInterpreterThunk (our unique offset!)
movz/movk x1, #addr           ; Load address of C++ StaticInterpreterThunk
add x0, sp, #16               ; X0 = pointer to saved registers
br  x3                        ; Jump to OUR unique offset in this same block

; At unique offset (Call[] array)
blr x1                        ; Call C++ StaticInterpreterThunk  
b   epilog                    ; Jump to epilog

; Epilog
ldp fp, lr, [sp], #80
ret
```

## The Problem

The ARM64 prologue:
1. Saves registers to stack ✓
2. Sets X0 = pointer to saved registers
3. Jumps to dynamic offset (still in generated code)
4. Dynamic offset calls X1 (C++ StaticInterpreterThunk)

But **StaticInterpreterThunk expects what in X0?**

Looking at the code:
```cpp
Var InterpreterStackFrame::StaticInterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...) {
    ARGUMENTS(args, callInfo);
    // ...
}
```

It expects:
- X0 = RecyclableObject* function (the function object!)
- X1 = CallInfo
- X2+ = varargs

But our prologue sets X0 = SP + 16 (pointer to saved registers), NOT the function object!

## The Solution

The prologue is OVERWRITING X0 before jumping to the call site. We need to:

1. **Option A**: Don't overwrite X0. The dynamic thunk call site will handle everything.
   - Remove `add x0, sp, #16`
   - The call site (`blr x1`) will call StaticInterpreterThunk with original X0

2. **Option B**: Use different register for pointer
   - Save pointer in X2 or other register
   - Let X0 remain as function object

3. **Option C**: Fix the C++ function signature
   - Make it accept pointer instead of function object
   - But this breaks compatibility

## Testing the Hypothesis

The issue is line: `add x0, sp, #16` before `br x3`.

This overwrites the function object pointer that StaticInterpreterThunk needs!

**Predicted fix**: Remove that line, or move the pointer to a different register.

