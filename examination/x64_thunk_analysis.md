# x64 JIT→Interpreter Thunk Call Analysis

## Overview

This document analyzes the x64 (AMD64) calling convention and code generation for JIT-compiled functions calling into the interpreter thunk in ChakraCore. This analysis will serve as a baseline for comparing against the ARM64 implementation to identify the hang issue.

---

## 1. x64 Calling Convention Summary

### Windows x64 ABI
- **Integer argument registers (in order):**
  - Arg 1: RCX
  - Arg 2: RDX
  - Arg 3: R8
  - Arg 4: R9
  - Additional args: Stack (right-to-left)

- **Floating-point argument registers:**
  - XMM0, XMM1, XMM2, XMM3 (first 4 float args)

- **Return value:** RAX (integer), XMM0 (float)

- **Stack alignment:** 16-byte aligned before `call`

- **Shadow space:** Caller must allocate 32 bytes (0x20) of "shadow space" (home space) for the first 4 register arguments, even if not used

### System V AMD64 ABI (Linux/macOS)
- **Integer argument registers (in order):**
  - Arg 1: RDI
  - Arg 2: RSI
  - Arg 3: RDX
  - Arg 4: RCX
  - Arg 5: R8
  - Arg 6: R9
  - Additional args: Stack

- **Floating-point argument registers:**
  - XMM0-XMM7 (first 8 float args)

- **Return value:** RAX (integer), XMM0 (float)

- **Stack alignment:** 16-byte aligned before `call`

- **No shadow space required** (major difference from Windows)

---

## 2. JavaScript Function Call Structure

When a JIT-compiled function calls another JavaScript function (or transitions to the interpreter), the following structure is used:

### Argument Layout (Windows x64)
```
Position | Register | Content
---------|----------|------------------
Arg 1    | RCX      | Function object pointer (JavascriptFunction*)
Arg 2    | RDX      | CallInfo (encoded: flags + arg count)
Arg 3    | R8       | this pointer
Arg 4    | R9       | First actual argument
[Stack]  | [RSP+n]  | Additional arguments...
```

### Argument Layout (System V x64)
```
Position | Register | Content
---------|----------|------------------
Arg 1    | RDI      | Function object pointer (JavascriptFunction*)
Arg 2    | RSI      | CallInfo (encoded: flags + arg count)
Arg 3    | RDX      | this pointer
Arg 4    | RCX      | First actual argument
Arg 5    | R8       | Second actual argument
Arg 6    | R9       | Third actual argument
[Stack]  | [RSP+n]  | Additional arguments...
```

---

## 3. Dynamic Interpreter Thunk Structure

ChakraCore uses **per-function dynamic interpreter thunks** when `DYNAMIC_INTERPRETER_THUNK` is enabled. Each `FunctionBody` gets its own thunk allocated from a thunk page.

### Key Components

1. **Thunk buffer:** A page of executable memory containing many small thunks
2. **Per-function thunk pointer:** `FunctionBody::m_dynamicInterpreterThunk`
3. **Thunk size:** Fixed size (96 bytes on x64 Windows, 64 bytes on System V)
4. **Entry point conversion:** `InterpreterThunkEmitter::ConvertToEntryPoint(m_dynamicInterpreterThunk)` converts the thunk address to a callable entry point

---

## 4. x64 Dynamic Interpreter Thunk Code (Windows)

From `InterpreterThunkEmitter.cpp` (Windows x64):

```x86asm
; Prolog - Home register arguments to stack (shadow space + spill area)
mov   qword ptr [rsp+10h], rdx    ; Home RDX (CallInfo) at RSP+0x10
mov   qword ptr [rsp+8],   rcx    ; Home RCX (function object) at RSP+0x08
mov   qword ptr [rsp+18h], r8     ; Home R8 (this) at RSP+0x18
mov   qword ptr [rsp+20h], r9     ; Home R9 (arg1) at RSP+0x20

; Load function info and dynamic thunk address
mov   rax, qword ptr [rcx+FunctionInfoOffset]           ; rax = function->functionInfo
mov   rcx, qword ptr [rax+FunctionProxyOffset]          ; rcx = functionInfo->functionProxy
mov   rdx, qword ptr [rcx+DynamicThunkAddressOffset]    ; rdx = functionProxy->m_dynamicInterpreterThunk

; Range check - verify thunk address is within allocated thunk buffer
and   rdx, 0xFFFFFFFFFFFFFFF8h    ; Force 8-byte alignment
mov   rcx, rdx
mov   rax, CallBlockStartAddress  ; Start of thunk buffer (patched at encode time)
sub   rcx, rax                    ; Offset from start
cmp   rcx, ThunkSize              ; Compare to total buffer size (patched)
jbe   safe                        ; Jump if within range
mov   rcx, errorcode              ; Load error code
int   29h                         ; Trigger breakpoint/error

safe:
; Setup argument: pointer to stack layout
lea   rcx, [rsp+8]                ; RCX = pointer to argument frame on stack
                                  ; This points to where we saved the function object

; Allocate stack space
sub   rsp, 28h                    ; Allocate 0x28 bytes (40 bytes)
                                  ; This maintains 16-byte alignment

; Load target and jump
mov   rax, <thunk>                ; rax = InterpreterStackFrame::InterpreterThunk (patched)
jmp   rdx                         ; Jump to the per-function dynamic thunk
                                  ; (which will then call/jump to the interpreter)
```

### Key Observations (Windows x64):

1. **Arguments are homed to stack** in the prolog
2. **A pointer to the stack frame is passed** in RCX (first argument)
3. The stack layout at `[rsp+8]` contains:
   ```
   [rsp+8]  = function object pointer (was in RCX)
   [rsp+10] = CallInfo (was in RDX)
   [rsp+18] = this pointer (was in R8)
   [rsp+20] = arg1 (was in R9)
   [rsp+28] = arg2 (if on stack)
   ...
   ```
4. This creates a **JavascriptCallStackLayout** structure on the stack
5. The dynamic thunk performs validation and then jumps to the actual interpreter
6. Stack is allocated (0x28 bytes) before the jump

---

## 5. x64 Dynamic Interpreter Thunk Code (System V / macOS)

From `InterpreterThunkEmitter.cpp` (System V AMD64):

```x86asm
; Prolog - setup stack frame
push  rbp                         ; Save frame pointer
mov   rbp, rsp                    ; Setup new frame

; Load function info and dynamic thunk address
mov   rax, qword ptr [rdi + FunctionInfoOffset]         ; rax = function->functionInfo (arg1 in RDI)
mov   rcx, qword ptr [rax+FunctionProxyOffset]          ; rcx = functionInfo->functionProxy
mov   rdx, qword ptr [rcx+DynamicThunkAddressOffset]    ; rdx = functionProxy->m_dynamicInterpreterThunk

; Range check - verify thunk address is within allocated thunk buffer
and   rdx, 0xfffffffffffffff8     ; Force 8-byte alignment
mov   rcx, rdx
mov   rax, CallBlockStartAddress  ; Start of thunk buffer (patched)
sub   rcx, rax                    ; Offset from start
cmp   rcx, ThunkSize              ; Compare to buffer size (patched)
jbe   safe                        ; Jump if within range
mov   rcx, errorcode              ; Load error code
int   29h                         ; Trigger error

safe:
; Setup argument: pointer to caller's stack (where args were passed)
lea   rdi, [rsp+0x10]             ; RDI = pointer to return address area + 16
                                  ; This points to where caller's args are on stack

; Load target and jump
mov   rax, <thunk>                ; rax = InterpreterStackFrame::InterpreterThunk (patched)
jmp   rdx                         ; Jump to dynamic thunk
```

### Key Observations (System V x64):

1. **Arguments stay in registers** until needed
2. **A pointer to the stack is passed** in RDI (first argument in System V ABI)
3. The pointer `[rsp+0x10]` points to the caller's argument area on the stack
4. System V expects the **caller** to have pushed arguments beyond register args onto the stack
5. **No explicit shadow space allocation** (System V doesn't use shadow space)
6. Stack frame is set up with push/mov rbp,rsp pattern

---

## 6. JIT Code Generation for Calls (x64)

### LowerCallI Flow (amd64/LowererMDArch.cpp)

```cpp
IR::Instr* LowererMDArch::LowerCallI(IR::Instr* callInstr, ushort callFlags, bool isHelper, ...)
{
    // 1. Unlink function object operand
    IR::Opnd* functionObjOpnd = callInstr->UnlinkSrc1();
    
    // 2. Generate function object validation (if needed)
    GenerateFunctionObjectTest(callInstr, functionObjOpnd->AsRegOpnd(), isHelper, ...);
    
    // 3. Generate pre-call setup
    GeneratePreCall(callInstr, functionObjOpnd, insertBeforeInstrForCFGCheck);
    
    // 4. Lower call arguments
    int32 argCount = LowerCallArgs(callInstr, callFlags, 1, &callInfo);
    
    // 5. Generate final CALL instruction
    IR::Instr* ret = this->LowerCall(callInstr, argCount);
    
    return ret;
}
```

### GeneratePreCall Flow

```cpp
void LowererMDArch::GeneratePreCall(IR::Instr* callInstr, IR::Opnd* functionObjOpnd, ...)
{
    // 1. Load function->type
    IR::RegOpnd* functionTypeRegOpnd = IR::RegOpnd::New(TyMachReg, m_func);
    IR::IndirOpnd* functionTypeIndirOpnd = 
        IR::IndirOpnd::New(functionObjOpnd->AsRegOpnd(), 
                          Js::DynamicObject::GetOffsetOfType(), TyMachReg, m_func);
    IR::Instr* mov = IR::Instr::New(Js::OpCode::MOV, functionTypeRegOpnd, 
                                    functionTypeIndirOpnd, m_func);
    insertBeforeInstrForCFGCheck->InsertBefore(mov);
    
    // Generated code:
    //   mov rax, qword ptr [function + offsetof(Type)]
    
    // 2. Load type->entryPoint
    entryPointIndirOpnd = 
        IR::IndirOpnd::New(functionTypeRegOpnd, 
                          Js::Type::GetOffsetOfEntryPoint(), TyMachPtr, m_func);
    mov = IR::Instr::New(Js::OpCode::MOV, entryPointRegOpnd, 
                        entryPointIndirOpnd, m_func);
    insertBeforeInstrForCFGCheck->InsertBefore(mov);
    
    // Generated code:
    //   mov rax, qword ptr [rax + offsetof(entryPoint)]
    
    // 3. Set entry point as call target
    callInstr->SetSrc1(entryPointRegOpnd);
    
    // 4. Setup first argument (function object)
    IR::Instr* instrMovArg1 = IR::Instr::New(Js::OpCode::MOV, 
                                             GetArgSlotOpnd(1), 
                                             functionObjOpnd, m_func);
    callInstr->InsertBefore(instrMovArg1);
    
    // Generated code (Windows):
    //   mov rcx, function_object_reg
    // Generated code (System V):
    //   mov rdi, function_object_reg
}
```

### LowerCallArgs Flow

```cpp
int32 LowererMDArch::LowerCallArgs(IR::Instr* callInstr, ushort callFlags, 
                                   Js::ArgSlot extraParams, IR::IntConstOpnd** callInfoOpndRef)
{
    // Walk the arg chain backwards from the call instruction
    // Each ArgOut instruction is converted to a MOV to the appropriate arg slot
    
    uint32 argCount = 0;
    IR::Instr* argInstr = callInstr;
    IR::Opnd* src2 = argInstr->UnlinkSrc2();
    
    while (src2->IsSymOpnd()) {
        StackSym* argLinkSym = src2->AsSymOpnd()->m_sym->AsStackSym();
        argInstr = argLinkSym->m_instrDef;
        src2 = argInstr->UnlinkSrc2();
        
        // Get argument position
        Js::ArgSlot argPosition = argInstr->GetDst()->AsSymOpnd()->m_sym->AsStackSym()->GetArgSlotNum();
        Js::ArgSlot index = argOffset + argPosition;
        
        // Convert ArgOut to assignment to arg slot
        IR::Opnd* dstOpnd = this->GetArgSlotOpnd(index, argLinkSym);
        argInstr->ReplaceDst(dstOpnd);
        
        // Generated code:
        //   mov [arg_register_or_stack], arg_value
        
        argCount++;
    }
    
    // Create CallInfo and place in second argument
    IR::IntConstOpnd* argCountOpnd = Lowerer::MakeCallInfoConst(callFlags, argCount, m_func);
    Lowerer::InsertMove(this->GetArgSlotOpnd(1 + extraParams), argCountOpnd, callInstr);
    
    // Generated code (Windows):
    //   mov rdx, callinfo_constant
    // Generated code (System V):
    //   mov rsi, callinfo_constant
    
    return argSlots;
}
```

### GetArgSlotOpnd - Argument Register Assignment

```cpp
IR::Opnd* LowererMDArch::GetArgSlotOpnd(uint16 index, StackSym* argSym, bool isHelper)
{
    // Map argument index to register or stack slot
    
    IRType type = argSym ? argSym->GetType() : TyMachReg;
    const bool isFloatArg = IRType_IsFloat(type) || IRType_IsSimd128(type);
    
    // Get register based on ABI
    RegNum reg = GetRegFromArgPosition(isFloatArg, index);
    
    if (reg != RegNOREG) {
        // Argument goes in register
        IR::RegOpnd* regOpnd = IR::RegOpnd::New(argSym, reg, type, m_func);
        regOpnd->m_isCallArg = true;
        return regOpnd;
    } else {
        // Argument goes on stack
        // ... create stack operand ...
    }
}
```

**GetRegFromArgPosition mapping:**

Windows x64:
```
Index 1 → RCX
Index 2 → RDX
Index 3 → R8
Index 4 → R9
Index 5+ → Stack
```

System V x64:
```
Index 1 → RDI
Index 2 → RSI
Index 3 → RDX
Index 4 → RCX
Index 5 → R8
Index 6 → R9
Index 7+ → Stack
```

---

## 7. Captured x64 Trace Example

From running `./build/chjitx64_debug/bin/ch/ch -TraceJitAsm examination/minimal_trace.js`:

```
Function: add
Address:  000000010D320000

Key instructions:
000000010D320039 | mov rax, qword ptr [rbp + 0x18]    ; Load CallInfo
000000010D32003D | and rax, 0xffffff                  ; Extract arg count
000000010D320044 | sub rax, 3                         ; Check arg count
000000010D320048 | jl  0x10d320088                    ; Jump if < 3 args
000000010D32004A | mov rdx, qword ptr [rbp + 0x30]    ; Load arg2
000000010D32004E | mov rcx, qword ptr [rbp + 0x28]    ; Load arg1
...
000000010D3200AB | movabs rdx, 0x7fb5c682c658         ; Load something (likely function/type)
000000010D3200AF | mov rsi, r11                        ; Setup arg (System V)
000000010D3200B2 | mov rdi, rcx                        ; Setup arg (System V)
000000010D3200B5 | movabs rax, 0x1123bd4c0            ; Load call target
000000010D3200BF | call rax                            ; Invoke call
```

---

## 8. Critical Path: JIT Call → Interpreter Thunk

### Step-by-step execution:

**1. JIT-compiled caller function:**
```x86asm
; Setup arguments for call
mov rcx, [function_object]        ; Arg 1: function object (Windows)
mov rdx, callinfo                 ; Arg 2: CallInfo
mov r8,  [this_ptr]               ; Arg 3: this
mov r9,  [arg1]                   ; Arg 4: first arg
; (Additional args on stack if needed)

; Load and call entry point
mov rax, [function_object + offsetof(type)]
mov rax, [rax + offsetof(entryPoint)]
call rax                          ; Call the entry point
```

**2. Entry point is a dynamic interpreter thunk:**
   - The `type->entryPoint` for an interpreted function points to its per-function dynamic thunk
   - The dynamic thunk is a small stub in executable memory

**3. Dynamic interpreter thunk (Windows):**
```x86asm
; Home arguments to stack
mov qword ptr [rsp+10h], rdx      ; Save CallInfo
mov qword ptr [rsp+8],   rcx      ; Save function object
mov qword ptr [rsp+18h], r8       ; Save this
mov qword ptr [rsp+20h], r9       ; Save arg1

; Validate and load actual interpreter thunk target
mov rax, qword ptr [rcx + FunctionInfo_offset]
mov rcx, qword ptr [rax + FunctionProxy_offset]
mov rdx, qword ptr [rcx + DynamicThunk_offset]
; ... range check ...

; Pass pointer to stack layout
lea rcx, [rsp+8]                  ; RCX = &stack_layout

; Allocate stack and jump
sub rsp, 28h
jmp rdx                           ; Jump to actual interpreter
```

**4. Actual interpreter thunk** (InterpreterStackFrame::InterpreterThunk):
   - Receives RCX pointing to JavascriptCallStackLayout on stack
   - Constructs InterpreterStackFrame
   - Begins bytecode interpretation

---

## 9. JavascriptCallStackLayout Structure

The stack layout that's created looks like this:

```cpp
// Conceptual structure (not actual C++ definition)
struct JavascriptCallStackLayout {
    JavascriptFunction* function;   // +0x00 (RCX was here)
    CallInfo callInfo;              // +0x08 (RDX was here)
    Var thisArg;                    // +0x10 (R8 was here)
    Var args[...];                  // +0x18... (R9, then stack args)
};
```

On Windows x64 with the thunk:
- `[rsp+8]` → function object
- `[rsp+10]` → CallInfo
- `[rsp+18]` → this
- `[rsp+20]` → arg1
- `[rsp+28]` → arg2
- etc.

The interpreter thunk receives `rcx = rsp+8`, which points to the base of this layout.

---

## 10. Key Design Principles (x64)

1. **Register arguments are immediately spilled to stack** in the dynamic thunk prolog
2. **A single pointer** to the complete argument layout is passed to the interpreter
3. **Stack-based calling convention** for the interpreter itself (pointer in RCX/RDI)
4. **Dynamic thunk performs validation** (range check) before jumping to interpreter
5. **Entry point indirection:** function→type→entryPoint→dynamic_thunk
6. **Stack alignment** is maintained throughout (16-byte aligned)
7. **Shadow space** is allocated on Windows (0x28 bytes)

---

## 11. Comparison Points for ARM64 Investigation

When comparing to ARM64, check:

1. **Are arguments being placed correctly?**
   - ARM64 uses X0-X7 for integer args, not RCX/RDX/R8/R9
   - X0 = function object, X1 = CallInfo, X2 = this, X3+ = args

2. **Is the stack layout being constructed properly?**
   - ARM64 might use STP (store pair) instructions
   - Check if offsets are correct (ARM64 is 8-byte pointers like x64)

3. **Is the pointer to the layout calculated correctly?**
   - ARM64 likely uses `add x0, sp, #offset` to create pointer
   - System V x64 uses `lea rdi, [rsp+0x10]`
   - Windows x64 uses `lea rcx, [rsp+8]`

4. **Are register saves/restores correct?**
   - ARM64 has different calling convention (X19-X28 are callee-saved)
   - X29 is frame pointer (FP), X30 is link register (LR)

5. **Is stack alignment maintained?**
   - ARM64 requires 16-byte stack alignment
   - Check SUB SP instructions

6. **Is the dynamic thunk offset/address correct?**
   - Check `m_dynamicInterpreterThunk` is being set correctly
   - Verify the offset calculation and range check

7. **Does the entry point conversion work?**
   - `InterpreterThunkEmitter::ConvertToEntryPoint()` must handle ARM64 correctly

---

## 12. Next Steps for ARM64 Debugging

1. **Capture ARM64 trace** (once hang is resolved enough to get partial trace):
   - Look for the sequence: load function, load type, load entryPoint, call
   - Check what registers are used for arguments

2. **Compare ARM64 dynamic thunk code**:
   - Read `InterpreterThunkEmitter.cpp` ARM64 sections
   - Verify it constructs the stack layout correctly
   - Verify the pointer calculation (should be `add x0, sp, #16` or similar)

3. **Check LowerMD.cpp (ARM64)**:
   - Verify `LowerCallI`, `GeneratePreCall`, `LowerCallArgs`
   - Confirm argument register mapping is correct
   - Verify stack setup matches thunk expectations

4. **Add instrumentation**:
   - Log `m_dynamicInterpreterThunk` address when allocated
   - Log entry point address when loaded
   - Log register values in ARM64 generated code before call
   - Compare logged addresses to verify they match

5. **Check for calling convention mismatches**:
   - Particularly around `DYNAMIC_INTERPRETER_THUNK` flag behavior
   - Verify ARM64 specific ABI compliance (AAPCS64)

---

## 13. References

- `ChakraCore/lib/Backend/amd64/LowererMDArch.cpp` - x64 lowering implementation
- `ChakraCore/lib/Backend/amd64/RegList.h` - x64 register definitions and argument mapping
- `ChakraCore/lib/Backend/InterpreterThunkEmitter.cpp` - Dynamic thunk generation
- `ChakraCore/lib/Runtime/Base/FunctionBody.cpp` - Function entry point management
- Windows x64 ABI: https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention
- System V AMD64 ABI: https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf