# x64 vs ARM64 Dynamic Interpreter Thunk Assembly Patterns

## Overview

This document provides side-by-side annotated assembly code for the dynamic interpreter thunk implementation on x64 and ARM64, highlighting the differences in calling conventions and stack management that may be causing the ARM64 hang.

---

## 1. Dynamic Interpreter Thunk: x64 Windows

### Assembly Code (from InterpreterThunkEmitter.cpp)

```x86asm
; ===== PROLOG: HOME REGISTER ARGUMENTS =====
; Windows x64 requires "shadow space" for register args
; Arguments arrive in: RCX=function, RDX=callinfo, R8=this, R9=arg1

0x00: 48 89 54 24 10        mov   qword ptr [rsp+0x10], rdx    ; Home RDX (callinfo)
0x05: 48 89 4C 24 08        mov   qword ptr [rsp+0x08], rcx    ; Home RCX (function)
0x0A: 4C 89 44 24 18        mov   qword ptr [rsp+0x18], r8     ; Home R8 (this)
0x0F: 4C 89 4C 24 20        mov   qword ptr [rsp+0x20], r9     ; Home R9 (arg1)

; At this point, stack layout is:
;   [rsp+0x08] = function object
;   [rsp+0x10] = CallInfo
;   [rsp+0x18] = this
;   [rsp+0x20] = arg1
;   [rsp+0x28] = arg2 (if on stack)
;   ... additional args

; ===== LOAD DYNAMIC THUNK ADDRESS =====
; Navigate: function → functionInfo → functionProxy → m_dynamicInterpreterThunk

0x14: 48 8B 41 00           mov   rax, qword ptr [rcx+0x00]    ; rax = function->functionInfo
                                                                 ; (FunctionInfoOffset patched)
0x18: 48 8B 48 00           mov   rcx, qword ptr [rax+0x00]    ; rcx = functionInfo->functionProxy
                                                                 ; (FunctionProxyOffset patched)
0x1C: 48 8B 51 00           mov   rdx, qword ptr [rcx+0x00]    ; rdx = proxy->m_dynamicInterpreterThunk
                                                                 ; (DynamicThunkAddressOffset patched)

; ===== RANGE CHECK FOR SECURITY =====
; Verify thunk address is within the allocated thunk buffer

0x20: 48 83 E2 F8           and   rdx, 0xFFFFFFFFFFFFFFF8      ; Force 8-byte alignment
0x24: 48 8B CA              mov   rcx, rdx                     ; rcx = aligned thunk address
0x27: 48 B8 [8 bytes]       mov   rax, CallBlockStartAddress   ; rax = start of thunk buffer
                                                                 ; (CallBlockStartAddrOffset - address patched)
0x31: 48 2B C8              sub   rcx, rax                     ; rcx = offset from start
0x34: 48 81 F9 [4 bytes]    cmp   rcx, ThunkSize               ; Compare to buffer size
                                                                 ; (ThunkSizeOffset - value patched)
0x3B: 76 09                 jbe   safe                         ; Jump if within range
0x3D: 48 C7 C1 [4 bytes]    mov   rcx, errorcode               ; Error code
0x44: CD 29                 int   0x29                         ; Trigger interrupt/breakpoint

; ===== SETUP CALL TO INTERPRETER =====
safe:
0x46: 48 8D 4C 24 08        lea   rcx, [rsp+0x08]              ; RCX = pointer to stack layout
                                                                 ; Points to where we saved function object
                                                                 ; This becomes Arg1 to interpreter thunk
0x4B: 48 83 EC 28           sub   rsp, 0x28                    ; Allocate 40 bytes stack space
                                                                 ; Maintains 16-byte alignment
                                                                 ; (StackAllocSize = 0x28)
0x4F: 48 B8 [8 bytes]       mov   rax, <thunk>                 ; rax = InterpreterStackFrame::InterpreterThunk
                                                                 ; (ThunkAddressOffset - address patched)
0x59: FF E2                 jmp   rdx                          ; Jump to dynamic thunk target
                                                                 ; (rdx still has m_dynamicInterpreterThunk)

; Note: After jmp, we're at the per-function dynamic thunk which will
; eventually call/jump to InterpreterStackFrame::InterpreterThunk with
; the stack pointer in RCX
```

### Key Points:

1. **Arguments arrive in registers:** RCX, RDX, R8, R9
2. **Immediately homed to stack** at fixed offsets (shadow space pattern)
3. **Pointer to stack layout** created via `lea rcx, [rsp+8]`
4. **Single argument** passed to interpreter: pointer in RCX
5. **Stack allocation:** 0x28 (40) bytes to maintain alignment
6. **Control flow:** JMP to dynamic thunk (tail call optimization)

---

## 2. Dynamic Interpreter Thunk: x64 System V (Linux/macOS)

### Assembly Code (from InterpreterThunkEmitter.cpp)

```x86asm
; ===== PROLOG: SETUP STACK FRAME =====
; System V doesn't use shadow space; arguments arrive in different registers
; Arguments: RDI=function, RSI=callinfo, RDX=this, RCX=arg1, R8=arg2, R9=arg3

0x00: 55                    push  rbp                          ; Save frame pointer
0x01: 48 89 E5              mov   rbp, rsp                     ; Setup new frame

; ===== LOAD DYNAMIC THUNK ADDRESS =====
; Navigate: function → functionInfo → functionProxy → m_dynamicInterpreterThunk

0x04: 48 8B 47 00           mov   rax, qword ptr [rdi+0x00]    ; rax = function->functionInfo
                                                                 ; RDI has function (Arg1 in System V)
                                                                 ; (FunctionInfoOffset patched)
0x08: 48 8B 48 00           mov   rcx, qword ptr [rax+0x00]    ; rcx = functionInfo->functionProxy
                                                                 ; (FunctionProxyOffset patched)
0x0C: 48 8B 51 00           mov   rdx, qword ptr [rcx+0x00]    ; rdx = proxy->m_dynamicInterpreterThunk
                                                                 ; (DynamicThunkAddressOffset patched)

; ===== RANGE CHECK FOR SECURITY =====

0x10: 48 83 E2 F8           and   rdx, 0xfffffffffffffff8      ; Force 8-byte alignment
0x14: 48 89 D1              mov   rcx, rdx                     ; rcx = aligned thunk address
0x17: 48 B8 [8 bytes]       mov   rax, CallBlockStartAddress   ; rax = start of thunk buffer
                                                                 ; (CallBlockStartAddressInstrOffset - patched)
0x21: 48 29 C1              sub   rcx, rax                     ; rcx = offset from start
0x24: 48 81 F9 [4 bytes]    cmp   rcx, ThunkSize               ; Compare to buffer size
                                                                 ; (CallThunkSizeInstrOffset - patched)
0x2B: 76 09                 jbe   safe                         ; Jump if within range
0x2D: 48 C7 C1 [4 bytes]    mov   rcx, errorcode               ; Error code
0x34: CD 29                 int   0x29                         ; Trigger error

; ===== SETUP CALL TO INTERPRETER =====
safe:
0x36: 48 8D 7C 24 10        lea   rdi, [rsp+0x10]              ; RDI = pointer to caller's stack
                                                                 ; Points 16 bytes above current RSP
                                                                 ; This is where return address + args are
                                                                 ; This becomes Arg1 to interpreter (System V)
0x3B: 48 B8 [8 bytes]       mov   rax, <thunk>                 ; rax = InterpreterStackFrame::InterpreterThunk
                                                                 ; (ThunkAddressOffset - address patched)
0x45: FF E2                 jmp   rdx                          ; Jump to dynamic thunk target

; Stack is already 16-byte aligned (after push rbp)
; No additional allocation needed
```

### Key Differences from Windows:

1. **Different argument registers:** RDI, RSI, RDX, RCX, R8, R9 (vs RCX, RDX, R8, R9 on Windows)
2. **No shadow space allocation** - registers stay in registers until needed
3. **Stack frame setup** with push rbp / mov rbp, rsp
4. **Pointer points to caller's stack** at `[rsp+0x10]` (return address area)
5. **No explicit stack allocation** before jump (already aligned)
6. **Arguments were on caller's stack** beyond 6 register args

---

## 3. Dynamic Interpreter Thunk: ARM64 (from InterpreterThunkEmitter.cpp)

### Assembly Code

```armasm
; ===== PROLOG: SAVE REGISTER ARGUMENTS =====
; ARM64 AAPCS64 calling convention:
; Arguments arrive in: X0=function, X1=callinfo, X2=this, X3=arg1, X4=arg2, ...

; Save register arguments to stack
0x00: [multiple STP instructions expected to save X0-X7 and X29/X30]
; Expected pattern (not shown in InterpreterThunkEmitter.cpp ARM section clearly):
;   stp x29, x30, [sp, #-16]!     ; Save FP and LR, pre-decrement SP
;   stp x0, x1, [sp, #-16]!       ; Save function and callinfo
;   stp x2, x3, [sp, #-16]!       ; Save this and arg1
;   ; etc. for more args

; ===== LOAD DYNAMIC THUNK ADDRESS =====
; Navigate: function → functionInfo → functionProxy → m_dynamicInterpreterThunk

; Load function->functionInfo
; X0 has function object pointer
ldr   x1, [x0, #FunctionInfoOffset]       ; x1 = function->functionInfo

; Load functionInfo->functionProxy
ldr   x2, [x1, #FunctionProxyOffset]      ; x2 = functionInfo->functionProxy

; Load proxy->m_dynamicInterpreterThunk
ldr   x3, [x2, #DynamicThunkAddressOffset]; x3 = proxy->m_dynamicInterpreterThunk

; ===== RANGE CHECK FOR SECURITY =====

and   x3, x3, #0xFFFFFFFFFFFFFFF8         ; Force 8-byte alignment
mov   x1, x3                               ; x1 = aligned thunk address
adr   x0, CallBlockStartAddress            ; x0 = start of thunk buffer
                                           ; Or: movz/movk to load 64-bit address
sub   x1, x1, x0                           ; x1 = offset from start
cmp   x1, ThunkSize                        ; Compare to buffer size
b.ls  safe                                 ; Branch if ≤ (unsigned)
; Error handling
mov   x0, #errorcode
brk   #0x29                                ; Breakpoint (ARM equivalent of int 0x29)

; ===== SETUP CALL TO INTERPRETER =====
safe:
; CRITICAL: Setup pointer to stack layout for interpreter
add   x0, sp, #16                          ; X0 = pointer to saved args on stack
                                           ; This should point to where we saved X0 (function)
                                           ; Offset depends on how many registers we saved

; Load interpreter thunk address
adr   x1, <thunk>                          ; x1 = InterpreterStackFrame::InterpreterThunk
                                           ; Or: movz/movk to load address

; Jump to dynamic thunk
br    x3                                   ; Branch (jump) to dynamic thunk

; Note: Stack alignment must be maintained (16-byte aligned)
; Note: ARM64 uses X29 (FP) and X30 (LR) instead of RBP and return address
```

### ARM64 Key Points:

1. **Arguments in X0-X7** (8 integer registers vs 4 on Windows x64, 6 on System V x64)
2. **Stack grows downward** with pre-decrement addressing
3. **STP (Store Pair)** instructions save register pairs efficiently
4. **Frame pointer X29** and **link register X30** must be saved
5. **Pointer calculation:** `add x0, sp, #offset` creates pointer to stack layout
6. **Control flow:** `br x3` instead of `jmp rdx`
7. **16-byte stack alignment** required (stricter than x64 in some respects)

---

## 4. Potential ARM64 Issues Based on Pattern Analysis

### Issue 1: Stack Layout Offset Calculation

**x64 Windows:**
```x86asm
lea rcx, [rsp+8]    ; Points to first saved argument (function)
```

**x64 System V:**
```x86asm
lea rdi, [rsp+0x10]  ; Points 16 bytes above SP (caller's area)
```

**ARM64 Expected:**
```armasm
add x0, sp, #16      ; Should point to where function was saved
```

**POTENTIAL BUG:** If ARM64 code does `add x0, sp, #16` but the actual saved arguments are at a different offset (e.g., if we saved X29/X30 first, they'd be at SP+0, and arguments would start at SP+16), the offset calculation could be wrong.

### Issue 2: Calling Convention Mismatch

**Windows x64:** Expects arguments homed in specific layout, pointer in RCX
**System V x64:** Expects arguments accessible via stack, pointer in RDI
**ARM64:** Should expect arguments accessible via stack, pointer in X0

**POTENTIAL BUG:** If the ARM64 interpreter thunk (the target of `br x3`) expects arguments in registers but the dynamic thunk passes a stack pointer, there's a mismatch.

### Issue 3: Stack Alignment

**x64:** 16-byte alignment before call, maintained by allocating 0x28 (Windows) or push rbp (System V)
**ARM64:** 16-byte alignment strictly enforced

**POTENTIAL BUG:** If ARM64 code doesn't maintain proper stack alignment when saving registers or doesn't allocate correct stack space, the jump to interpreter might fault.

### Issue 4: Register Save/Restore Pattern

**x64:** Only saves what's needed (no explicit FP save in Windows thunk)
**ARM64:** MUST save X29 (FP) and X30 (LR) at function entry per AAPCS64

**POTENTIAL BUG:** If ARM64 dynamic thunk doesn't properly save/restore X29/X30, return from interpreter will be broken.

### Issue 5: Dynamic Thunk Address Storage

**x64:** `m_dynamicInterpreterThunk` stored as offset or absolute address
**ARM64:** Must be absolute address (no PC-relative jumps with `br`)

**POTENTIAL BUG:** If `m_dynamicInterpreterThunk` is stored as an offset on ARM64 but treated as absolute address, the `br x3` will jump to invalid location.

### Issue 6: Pointer Arithmetic

**x64:** All pointer arithmetic is 64-bit, straightforward
**ARM64:** Same 64-bit pointers, but different instruction encoding

**POTENTIAL BUG:** If the offset calculation `add x0, sp, #offset` uses the wrong offset value (hardcoded from x64), the pointer will be invalid.

---

## 5. Debug Checklist for ARM64 Hang

Use this checklist to systematically verify each component:

### A. Entry Point Setup

- [ ] Verify `FunctionBody::GenerateDynamicInterpreterThunk()` is called on ARM64
- [ ] Verify `m_dynamicInterpreterThunk` is set to a valid address
- [ ] Verify `SetOriginalEntryPoint()` is called with correct address
- [ ] Log the actual address stored in `m_dynamicInterpreterThunk`
- [ ] Verify `InterpreterThunkEmitter::ConvertToEntryPoint()` works correctly on ARM64

### B. JIT Code Generation

- [ ] Check `LowerMD.cpp` (ARM64) `LowerCallI()` implementation
- [ ] Verify arguments are placed in X0, X1, X2, X3, etc.
- [ ] Verify `GeneratePreCall()` loads function->type->entryPoint correctly
- [ ] Verify the call/branch instruction targets the entry point
- [ ] Capture actual ARM64 assembly for a warmed function call

### C. Dynamic Thunk Code

- [ ] Verify ARM64 dynamic thunk code matches the expected pattern
- [ ] Check that STP instructions save X0-X7 to stack
- [ ] Verify X29 (FP) and X30 (LR) are saved
- [ ] Check the offset calculation: `add x0, sp, #???`
- [ ] Verify the offset matches where arguments were saved
- [ ] Verify stack alignment is maintained (multiple of 16)

### D. Stack Layout

- [ ] Verify stack layout after STP instructions matches expectations
- [ ] Check that `[sp+offset]` points to saved function object
- [ ] Verify CallInfo is at correct offset
- [ ] Verify this pointer is at correct offset
- [ ] Verify argument array is contiguous and accessible

### E. Control Flow

- [ ] Verify `br x3` jumps to correct address (within thunk buffer)
- [ ] Verify range check passes (offset < ThunkSize)
- [ ] Verify target address is executable memory
- [ ] Check that after `br`, execution continues in interpreter thunk
- [ ] Verify interpreter thunk can read arguments from stack pointer

### F. Calling Convention

- [ ] Verify ARM64 calling convention matches what interpreter expects
- [ ] Check AAPCS64 compliance (X0-X7 for args, X29=FP, X30=LR)
- [ ] Verify no register clobbering before interpreter call
- [ ] Check that interpreter receives pointer in X0
- [ ] Verify stack is 16-byte aligned when interpreter starts

---

## 6. Instrumentation Points

Add logging at these points to diagnose the hang:

```cpp
// In FunctionBody::GenerateDynamicInterpreterThunk()
fprintf(stderr, "[ARM64] m_dynamicInterpreterThunk = %p\n", m_dynamicInterpreterThunk);
fprintf(stderr, "[ARM64] entryPoint = %p\n", GetOriginalEntryPoint());

// In InterpreterThunkEmitter::EncodeInterpreterThunk() (ARM64 section)
fprintf(stderr, "[ARM64] Encoding thunk at %p, size=%d\n", dest, ThunkSize);
fprintf(stderr, "[ARM64] CallBlockStartAddress = %p\n", callBlockStartAddress);
fprintf(stderr, "[ARM64] StaticThunkAddress = %p\n", staticThunkAddress);

// In LowerMD.cpp (ARM64) GeneratePreCall()
// Add instruction to print X0 before call
// mov x10, x0
// bl _print_register_x10
// mov x0, x10

// In InterpreterStackFrame::InterpreterThunk (ARM64 entry)
fprintf(stderr, "[ARM64 INT] Entry, X0=%p\n", <X0 value>);
fprintf(stderr, "[ARM64 INT] Stack layout: func=%p, callinfo=%lx\n", 
        layout->function, layout->callInfo);
```

---

## 7. Expected ARM64 Dynamic Thunk Pattern (Corrected)

Based on the x64 patterns and AAPCS64, the ARM64 dynamic thunk should look like:

```armasm
; ===== PROLOG: SAVE ALL ARGUMENTS TO STACK =====
; Create JsCallStackLayout on stack
; SP must be 16-byte aligned throughout

stp   x29, x30, [sp, #-16]!      ; Save FP and LR, pre-decrement by 16
stp   x0, x1, [sp, #-16]!        ; Save X0 (function) and X1 (callinfo)
stp   x2, x3, [sp, #-16]!        ; Save X2 (this) and X3 (arg1)
stp   x4, x5, [sp, #-16]!        ; Save X4 (arg2) and X5 (arg3) if needed
stp   x6, x7, [sp, #-16]!        ; Save X6 (arg4) and X7 (arg5) if needed

; At this point:
;   [sp+0]  = x6, x7 (if saved)
;   [sp+16] = x4, x5 (if saved)
;   [sp+32] = x2 (this), x3 (arg1)
;   [sp+48] = x0 (function), x1 (callinfo)
;   [sp+64] = x29, x30

; ===== LOAD DYNAMIC THUNK ADDRESS =====
; X0 still has function object (or reload from [sp+48])
ldr   x0, [sp, #48]              ; Reload X0 = function (if clobbered)
ldr   x1, [x0, #FunctionInfo_offset]
ldr   x2, [x1, #FunctionProxy_offset]
ldr   x3, [x2, #DynamicThunk_offset]

; ===== RANGE CHECK =====
and   x3, x3, #-8                ; 8-byte align
mov   x1, x3
adrp  x0, CallBlockStart@PAGE    ; Load page of start address
add   x0, x0, CallBlockStart@PAGEOFF
sub   x1, x1, x0
mov   x2, ThunkSize
cmp   x1, x2
b.ls  safe
mov   x0, #ERROR_CODE
brk   #0

; ===== SETUP CALL =====
safe:
add   x0, sp, #48                ; X0 = pointer to saved function object
                                 ; This creates the argument for interpreter
                                 ; Interpreter will read [x0] = function,
                                 ; [x0+8] = callinfo, etc.

; No additional stack allocation needed (already allocated in prolog)

br    x3                         ; Jump to dynamic thunk / interpreter

; Note: After this point, we're in interpreter which will:
; 1. Read arguments from [X0], [X0+8], [X0+16], etc.
; 2. Build InterpreterStackFrame
; 3. Execute bytecode
; 4. Return via X30 (LR) which we saved at [sp+64+8]
```

**Critical calculation:** `add x0, sp, #48` points to where we saved X0 (function object).

If ARM64 code is using a different offset (e.g., #16), it would point to X2/X3, not X0/X1, causing the interpreter to receive wrong values!

---

## 8. Comparison Summary Table

| Aspect | x64 Windows | x64 System V | ARM64 AAPCS64 |
|--------|-------------|--------------|---------------|
| **Arg registers** | RCX, RDX, R8, R9 | RDI, RSI, RDX, RCX, R8, R9 | X0-X7 |
| **FP register** | RBP | RBP | X29 |
| **LR/Return** | (stack) | (stack) | X30 |
| **Shadow space** | Yes (32 bytes) | No | No |
| **Stack alignment** | 16 bytes | 16 bytes | 16 bytes (strict) |
| **Pointer to layout** | `lea rcx, [rsp+8]` | `lea rdi, [rsp+0x10]` | `add x0, sp, #???` |
| **Stack alloc** | `sub rsp, 0x28` | None (push rbp) | Via STP pre-decrement |
| **Control flow** | `jmp rdx` | `jmp rdx` | `br x3` |
| **Save pairs** | MOV individual | MOV individual | STP (store pair) |

**The critical difference:** The offset in `add x0, sp, #offset` must be calculated based on how many registers were saved with STP. Each STP saves 16 bytes, so if we did 5 STPs (X29/X30, X0/X1, X2/X3, X4/X5, X6/X7), we need to add `sp, #(4*16)=64` to point to where X0/X1 were saved. But if we only saved 3 pairs, it's `sp, #(2*16)=32`.

**Most likely bug:** Hard-coded offset from x64 (e.g., #16 from System V) used on ARM64, but ARM64 actually saved registers at different offset due to different save pattern.

---

## Next Action

**Run ARM64 under debugger and examine:**
1. Actual ARM64 dynamic thunk code bytes
2. SP value at entry
3. Where arguments are actually saved on stack
4. What offset is used in `add x0, sp, #?`
5. What address is in X0 when jumping to interpreter
6. What values the interpreter reads from [X0], [X0+8], etc.

Compare these values against expected values to identify the mismatch.