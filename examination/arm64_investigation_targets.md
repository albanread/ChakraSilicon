# ARM64 Investigation: Code Locations and Checklist

## Overview

This document provides a quick reference for all the code locations that need to be examined to debug the ARM64 JIT→Interpreter hang issue.

---

## 1. Dynamic Thunk Generation

### InterpreterThunkEmitter.cpp

**Location:** `ChakraCore/lib/Backend/InterpreterThunkEmitter.cpp`

**What to check:**
- ARM64 thunk assembly code (search for `#elif defined(_M_ARM64)` or `#ifdef _M_ARM64`)
- Verify the thunk pattern matches expected AAPCS64 calling convention
- Check offset calculations for patching
- Verify stack layout creation (STP instructions)
- Check the `add x0, sp, #offset` instruction

**Key offsets to verify:**
```cpp
constexpr BYTE FunctionInfoOffset = ???;
constexpr BYTE FunctionProxyOffset = ???;
constexpr BYTE DynamicThunkAddressOffset = ???;
constexpr BYTE CallBlockStartAddrOffset = ???;
constexpr BYTE ThunkSizeOffset = ???;
constexpr BYTE ThunkAddressOffset = ???;
```

**Expected pattern (validate against actual code):**
```armasm
stp x29, x30, [sp, #-16]!    ; Save FP/LR
stp x0, x1, [sp, #-16]!      ; Save function, callinfo
stp x2, x3, [sp, #-16]!      ; Save this, arg1
; ... load dynamic thunk address ...
; ... range check ...
add x0, sp, #OFFSET          ; ← CHECK THIS OFFSET!
br x3
```

---

## 2. ARM64 JIT Code Generation

### LowerMD.cpp (ARM64)

**Location:** `ChakraCore/lib/Backend/arm64/LowerMD.cpp`

**Functions to examine:**

#### LowerCallI
```cpp
IR::Instr* LowererMD::LowerCallI(IR::Instr* callInstr, ushort callFlags, bool isHelper, ...)
```

**What to check:**
- How arguments are loaded into X0-X7
- How function object is prepared
- How entry point is loaded
- The actual call/branch instruction generated

#### GeneratePreCall
```cpp
void LowererMD::GeneratePreCall(IR::Instr* callInstr, IR::Opnd* functionObjOpnd, ...)
```

**What to check:**
- Loading `function->type`
- Loading `type->entryPoint`
- Setting up X0 with function object
- Generated instruction sequence

#### LowerCallArgs
```cpp
int32 LowererMD::LowerCallArgs(IR::Instr* callInstr, ushort callFlags, Js::ArgSlot extraParams, ...)
```

**What to check:**
- How CallInfo is constructed
- How arguments are mapped to X0, X1, X2, X3, X4, X5, X6, X7
- How additional arguments are pushed to stack
- Whether stack alignment is maintained

---

## 3. Register Definitions

### RegList.h (ARM64)

**Location:** `ChakraCore/lib/Backend/arm64/RegList.h`

**What to check:**
- Verify ARM64 register definitions
- Check argument register mapping macros:
  ```cpp
  REG_INT_ARG(0, ???)  // Should be X0
  REG_INT_ARG(1, ???)  // Should be X1
  REG_INT_ARG(2, ???)  // Should be X2
  // ... etc
  ```

**Expected ARM64 integer argument registers:**
- X0, X1, X2, X3, X4, X5, X6, X7

---

## 4. Function Entry Point Management

### FunctionBody.cpp

**Location:** `ChakraCore/lib/Runtime/Base/FunctionBody.cpp`

**Function to examine:**
```cpp
void FunctionBody::GenerateDynamicInterpreterThunk()
```

**What to check (around line 3818):**
- Verify `m_dynamicInterpreterThunk` is set
- Check `scriptContext->GetNextDynamicInterpreterThunk(&m_dynamicInterpreterThunk)` is called
- Verify `SetOriginalEntryPoint()` is called with correct address
- Check `InterpreterThunkEmitter::ConvertToEntryPoint()` is used

**Add instrumentation:**
```cpp
#ifdef _M_ARM64
fprintf(stderr, "[ARM64] GenerateDynamicInterpreterThunk: m_dynamicInterpreterThunk=%p\n", 
        m_dynamicInterpreterThunk);
fprintf(stderr, "[ARM64] GenerateDynamicInterpreterThunk: entryPoint=%p\n", 
        GetOriginalEntryPoint_Unchecked());
#endif
```

---

## 5. Thunk Allocation

### ScriptContext.cpp

**Location:** `ChakraCore/lib/Runtime/Base/ScriptContext.cpp`

**Function to examine:**
```cpp
BYTE* ScriptContext::GetNextDynamicInterpreterThunk(BYTE** thunk)
```

**What to check:**
- Verify thunk buffer allocation on ARM64
- Check that thunk size matches `InterpreterThunkEmitter::ThunkSize`
- Verify thunk address is executable
- Check alignment requirements

---

## 6. Interpreter Entry Point

### InterpreterStackFrame.cpp

**Location:** `ChakraCore/lib/Runtime/Language/InterpreterStackFrame.cpp`

**Function to examine:**
```cpp
Js::Var InterpreterStackFrame::InterpreterThunk(JavascriptCallStackLayout* layout)
```

**What to check:**
- Verify this function receives correct pointer in X0 (on ARM64)
- Check that `layout->function` is valid
- Check that `layout->callInfo` is valid
- Verify stack frame construction

**Add instrumentation:**
```cpp
#ifdef _M_ARM64
fprintf(stderr, "[ARM64 INT] InterpreterThunk: layout=%p\n", layout);
fprintf(stderr, "[ARM64 INT] function=%p, callInfo=%lx\n", 
        layout->function, layout->callInfo);
#endif
```

---

## 7. Static Interpreter Thunk

### InterpreterStackFrame.h

**Location:** `ChakraCore/lib/Runtime/Language/InterpreterStackFrame.h`

**What to check:**
- Declaration of `StaticInterpreterThunk` for ARM64
- Verify signature matches dynamic thunk expectations

---

## 8. Type System

### Type.h / Type.cpp

**Location:** `ChakraCore/lib/Runtime/Types/Type.h`

**What to check:**
- `Type::GetOffsetOfEntryPoint()` returns correct offset
- Entry point field is at correct location in Type structure
- Verify offset is same across architectures (should be)

---

## 9. Encoder (ARM64)

### EncoderMD.cpp (ARM64)

**Location:** `ChakraCore/lib/Backend/arm64/EncoderMD.cpp`

**What to check:**
- How CALL/BL/BR instructions are encoded
- How indirect calls are handled
- Register allocation for call targets
- Stack frame setup/teardown

---

## 10. Key Configuration Flags

### ConfigFlagsList.h

**Location:** `ChakraCore/lib/Common/ConfigFlagsList.h`

**Flags to check:**
- `DYNAMIC_INTERPRETER_THUNK` - verify it's enabled for ARM64
- Any ARM64-specific flags that might affect thunk generation

### FunctionBody.h

**Location:** `ChakraCore/lib/Runtime/Base/FunctionBody.h`

**What to check:**
```cpp
#if DYNAMIC_INTERPRETER_THUNK
    BYTE* m_dynamicInterpreterThunk;
#endif
```

Verify this is compiled in for ARM64 builds.

---

## 11. Debugging Strategy

### Step 1: Verify Dynamic Thunk Allocation
```bash
# Add to FunctionBody::GenerateDynamicInterpreterThunk()
fprintf(stderr, "[ARM64] Function: %s\n", GetDisplayName());
fprintf(stderr, "[ARM64] m_dynamicInterpreterThunk: %p\n", m_dynamicInterpreterThunk);
fprintf(stderr, "[ARM64] EntryPoint: %p\n", GetOriginalEntryPoint());
```

### Step 2: Verify JIT Generated Code
```bash
# Run with trace enabled
./build/chjita64_debug/bin/ch/ch -TraceJitAsm test.js
```

Look for:
- Instructions loading function object into X0
- Instructions loading type into X1/X2
- Instructions loading entryPoint into X3/X4
- BL or BR instruction

### Step 3: Examine Dynamic Thunk at Runtime
```bash
# Run under LLDB
lldb ./build/chjita64_debug/bin/ch/ch
(lldb) run test.js
# When hung, break
(lldb) bt
(lldb) register read
(lldb) memory read -c 96 $x3    # Read 96 bytes at thunk address (in X3)
(lldb) disassemble -s $x3 -c 24 # Disassemble thunk
```

### Step 4: Examine Stack Layout
```bash
(lldb) register read sp
(lldb) memory read -c 128 $sp   # Read stack
(lldb) x/16gx $sp               # Display as 16 8-byte words
```

Expected stack layout (if using pattern from Windows x64 adapted):
```
[sp+0]  : saved X6, X7
[sp+16] : saved X4, X5
[sp+32] : saved X2 (this), X3 (arg1)
[sp+48] : saved X0 (function), X1 (callinfo)
[sp+64] : saved X29 (FP), X30 (LR)
```

### Step 5: Check Pointer Calculation
```bash
# If we see: add x0, sp, #OFFSET
# OFFSET should be 48 (to point to saved X0)
# Verify with:
(lldb) register read x0
(lldb) register read sp
# x0 should equal sp + 48
```

---

## 12. Specific Bugs to Look For

### Bug #1: Wrong Stack Offset
**Symptom:** Interpreter receives garbage values
**Check:** 
- `add x0, sp, #OFFSET` in dynamic thunk
- Compare OFFSET to actual location where X0 was saved

### Bug #2: Calling Convention Mismatch
**Symptom:** Hang or crash immediately after branch
**Check:**
- Verify X0-X7 are used for arguments (not X1-X8 or different order)
- Verify interpreter expects pointer in X0

### Bug #3: Stack Misalignment
**Symptom:** Crash with alignment fault
**Check:**
- SP must be 16-byte aligned before BR
- Count number of STP instructions (each decrements SP by 16)
- Verify total decrement is multiple of 16

### Bug #4: Wrong Entry Point Address
**Symptom:** Jump to invalid address
**Check:**
- `m_dynamicInterpreterThunk` value
- Range check passes
- Address is within thunk buffer

### Bug #5: Entry Point Not Set
**Symptom:** Hang because calling wrong function
**Check:**
- `SetOriginalEntryPoint()` was called
- `type->entryPoint` points to dynamic thunk (not static thunk)

### Bug #6: Thunk Not Encoded
**Symptom:** Execute garbage/zeros
**Check:**
- `InterpreterThunkEmitter::EncodeInterpreterThunk()` was called
- Thunk buffer is executable
- Thunk bytes match expected pattern

---

## 13. Comparison with Working x64

For each item above, compare:
1. **x64 code path** (working) in `lib/Backend/amd64/`
2. **ARM64 code path** (broken) in `lib/Backend/arm64/`

Key files to diff:
```bash
diff -u ChakraCore/lib/Backend/amd64/LowererMDArch.cpp \
        ChakraCore/lib/Backend/arm64/LowerMD.cpp

# Look for GeneratePreCall, LowerCallI, LowerCallArgs
```

---

## 14. Quick Test Case

Minimal test to reproduce hang:
```javascript
// minimal_hang_test.js
function add(a, b) {
    return a + b;
}

// Warm up to force JIT compilation
for (let i = 0; i < 100; i++) {
    add(i, i+1);
}

// This call should go through JIT → Interpreter transition
print("Result: " + add(1, 2));
```

Run:
```bash
./build/chjita64_debug/bin/ch/ch minimal_hang_test.js
# Should hang on ARM64
```

---

## 15. Expected Fix Location

Based on analysis, the fix is most likely in ONE of these locations:

1. **InterpreterThunkEmitter.cpp (ARM64 section)**
   - Wrong offset in `add x0, sp, #OFFSET`
   - Fix: Calculate correct offset based on STP count

2. **LowerMD.cpp (ARM64) GeneratePreCall()**
   - Wrong register used for function object
   - Fix: Ensure X0 = function, not X1 or other register

3. **LowerMD.cpp (ARM64) LowerCallArgs()**
   - Arguments in wrong registers
   - Fix: Use X0-X7 in correct order

4. **FunctionBody.cpp GenerateDynamicInterpreterThunk()**
   - Not setting entry point correctly on ARM64
   - Fix: Add ARM64-specific path

---

## 16. Once Fixed, Verify

- [ ] Minimal test passes
- [ ] All warmup tests pass
- [ ] No performance regression
- [ ] Trace output shows correct addresses
- [ ] Stack layout is correct
- [ ] No memory corruption

---

## 17. Documentation to Update

After fix:
- Document ARM64 calling convention differences
- Update comments in InterpreterThunkEmitter.cpp
- Add test case to regression suite
- Update build documentation