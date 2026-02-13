;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

;Var arm64_CallFunction(JavascriptFunction* function, CallInfo info, uint argCount, Var* values, JavascriptMethod entryPoint)
;
;   This method should be called as follows
;       varResult = arm64_CallFunction((JavascriptFunction*)function, args.Info, argCount, args.Values, entryPoint);
;
;   and makes the following call
;           return entryPoint(function, info, values[0], values[1], ..., values[n-2], values[n-1]);
;   where n = info.Count
;
;   The JIT calling convention on ARM64 expects:
;       x0 = function object
;       x1 = callInfo
;       x2 = values[0] ("this")
;       x3 = values[1] (first user arg)
;       x4..x7 = values[2..5] (more args)
;       stack  = [function, callInfo, values[0], values[1], ...] (full arg frame for stack walker)
;
;   arm64_CallFunction puts all args on the stack (for the stack walker / va_list) AND loads
;   values[0..5] into x2-x7 so that the JIT prologue can home them correctly.
;

#include "unixasmmacros.inc"

NESTED_ENTRY arm64_CallFunction, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -16    ; save FP/LR registers, implicitly stores FP in SP

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
    ; It is safe to load beyond the actual arg count because:
    ;   (a) the memory is within our stack frame (mapped and accessible), and
    ;   (b) the JIT/interpreter uses callInfo.Count to know the real arg count.
    ; We always allocated at least 16 bytes, and the reads at sp+0x10..sp+0x38
    ; fall within our frame or the saved fp/lr area above.
    ldp     x2, x3, [sp, #0x10]                ; x2 = values[0], x3 = values[1]
    ldp     x4, x5, [sp, #0x20]                ; x4 = values[2], x5 = values[3]
    ldp     x6, x7, [sp, #0x30]                ; x6 = values[4], x7 = values[5]
    ; x0 = function, x1 = callInfo are still intact from entry

    blr     x9                                  ; call entry point (saved in x9)

    mov     sp, fp                              ; explicitly restore sp
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 16  ; restore FP/LR
    ret                                         ; return

NESTED_END arm64_CallFunction

;-------------------------------------------------------------------------------------------------------
; arm64_CallJIT: Trampoline for interpreter → JIT-compiled function calls on Apple ARM64.
;
; This uses the "overflow only" stack layout matching Windows arm64_CallFunction.asm:
;   - x0-x7 carry all register arguments (function, callInfo, values[0..5])
;   - Only overflow arguments (values[6+]) are placed on the stack at [SP+0], [SP+8], ...
;   - The JIT callee's prolog homes x0-x7 and reads overflow args from [CallerSP+0]
;
; On entry (matches arm64_CallFunction signature):
;   x0 = function, x1 = callInfo, x2 = argCount, x3 = values, x4 = entryPoint
;
NESTED_ENTRY arm64_CallJIT, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -16    ; save FP/LR, SP -= 16, FP = SP

    mov     x8, x4                              ; save entryPoint in x8
    mov     x9, x3                              ; save values pointer in x9

    ; Check if argCount > 6 (overflow args needed on stack)
    subs    x5, x2, #6                          ; x5 = argCount - 6
    b.gt    LOCAL_LABEL(JIT_StackAlloc)          ; if overflow, allocate stack

    ; argCount <= 6: No stack args needed. Use computed branch to load only needed values.
    ; Each ldr instruction is exactly 4 bytes on ARM64.
    adr     x5, LOCAL_LABEL(JIT_CopyZero)       ; address of end of copy sequence
    sub     x5, x5, x2, lsl #2                  ; compute start: CopyZero - argCount*4
    br      x5                                  ; branch to start of loads

LOCAL_LABEL(JIT_CopyAll):
    ldr     x7, [x9, #40]                       ; values[5] → x7
    ldr     x6, [x9, #32]                       ; values[4] → x6
    ldr     x5, [x9, #24]                       ; values[3] → x5
    ldr     x4, [x9, #16]                       ; values[2] → x4
    ldr     x3, [x9, #8]                        ; values[1] → x3
    ldr     x2, [x9, #0]                        ; values[0] → x2

LOCAL_LABEL(JIT_CopyZero):
    blr     x8                                  ; call entry point

    mov     sp, fp                              ; restore SP
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 16  ; restore FP/LR
    ret

LOCAL_LABEL(JIT_StackAlloc):
    ; Overflow args: allocate aligned stack space for (argCount - 6) values
    add     x15, x5, #1                         ; round up to even
    lsr     x15, x15, #1                        ; divide by 2
    sub     sp, sp, x15, lsl #4                 ; allocate (16-byte aligned chunks)

    ; Copy overflow args: values[6..N-1] to [SP+0], [SP+8], ...
    add     x10, x9, #48                        ; x10 = source = &values[6]
    mov     x11, sp                             ; x11 = dest = SP

LOCAL_LABEL(JIT_CopyOverflow):
    ldr     x12, [x10], #8                      ; read from source
    str     x12, [x11], #8                      ; store to dest
    subs    x5, x5, #1                          ; decrement overflow count
    b.ne    LOCAL_LABEL(JIT_CopyOverflow)        ; loop until done

    ; Load values[0..5] into x2-x7 (always 6 values for >6 arg case)
    b       LOCAL_LABEL(JIT_CopyAll)             ; jump to load all 6 register values

NESTED_END arm64_CallJIT

;-------------------------------------------------------------------------------------------------------
; arm64_CallDirectVarargs: Trampoline for DarwinPCS variadic Entry* calls from JIT CallDirect.
;
; Problem: On DarwinPCS (Apple ARM64), variadic function arguments are always passed on the stack,
;          never in registers. But the JIT places CallDirect arguments in registers x0-x7 (with
;          overflow on the stack for 8+ total args). When the variadic callee uses va_start, it
;          reads from the caller's stack area — which doesn't have the register-passed args.
;
; Solution: This trampoline receives args in registers (as the JIT emits them), builds a contiguous
;           stack frame [function, callInfo, arg0, arg1, ...], and then calls the real Entry*
;           function. The callee's va_start then reads the correct values from our stack frame.
;
; On entry:
;   x0      = function (RecyclableObject*)
;   x1      = callInfo (Count:24 | Flags:8 | unused:32)
;   x2-x7   = first 6 variadic Var* args (values[0..5])
;   [SP+0]  = overflow args (values[6+]), if Count > 6
;   x16     = real target Entry* function address
;   LR      = return address in JIT code
;
; Stack frame built for callee (at our SP before BLR x16):
;   [SP+0]  = function   [SP+8]  = callInfo
;   [SP+16] = values[0]  [SP+24] = values[1]
;   [SP+32] = values[2]  [SP+40] = values[3]
;   [SP+48] = values[4]  [SP+56] = values[5]
;   [SP+64] = values[6]  ... (overflow copies)
;
; The callee's DECLARE_ARGS_VARARRAY does:
;   va_start(_vl, callInfo)  → _vl points to [SP+0] = function
;   va = (Var*)_vl + 2       → skips function + callInfo → points to values[0]
;
NESTED_ENTRY arm64_CallDirectVarargs, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -16    ; save FP/LR, SP -= 16, FP = SP

    ; Extract JS argument count from callInfo (lower 24 bits)
    and     x9, x1, #0xFFFFFF      ; x9 = Count (number of JS args including 'this')

    ; Total stack slots needed = Count + 2 (function + callInfo + Count values)
    ; Always allocate at least 8 slots (64 bytes) so we can unconditionally
    ; store all 8 register args via STP without bounds checks.
    add     x10, x9, #2            ; x10 = Count + 2
    cmp     x10, #8
    mov     x11, #8
    csel    x10, x10, x11, gt      ; x10 = max(Count + 2, 8)

    ; Round up to even number of slots for 16-byte stack alignment
    add     x10, x10, #1
    bic     x10, x10, #1           ; x10 = even-aligned slot count

    ; Allocate outgoing stack area
    lsl     x10, x10, #3           ; convert slots to bytes
    sub     sp, sp, x10            ; allocate

    ; Store all 8 register args unconditionally (we have at least 64 bytes)
    stp     x0, x1, [sp, #0]       ; [SP+0]  = function, [SP+8]  = callInfo
    stp     x2, x3, [sp, #16]      ; [SP+16] = arg0,     [SP+24] = arg1
    stp     x4, x5, [sp, #32]      ; [SP+32] = arg2,     [SP+40] = arg3
    stp     x6, x7, [sp, #48]      ; [SP+48] = arg4,     [SP+56] = arg5

    ; Copy overflow args from the JIT's outgoing area (if Count > 6).
    ; The JIT's overflow args are at [FP + 16 + 0], [FP + 16 + 8], ...
    ; (FP points to our saved FP/LR pair; +16 = original SP before trampoline entry)
    subs    x11, x9, #6            ; x11 = overflow count = Count - 6
    b.le    LOCAL_LABEL(DirectNoOverflow)

    add     x12, fp, #16           ; x12 = source: JIT's overflow area
    add     x13, sp, #64           ; x13 = dest: after the 8 register shadow slots

LOCAL_LABEL(DirectCopyLoop):
    ldr     x14, [x12], #8
    str     x14, [x13], #8
    subs    x11, x11, #1
    b.gt    LOCAL_LABEL(DirectCopyLoop)

LOCAL_LABEL(DirectNoOverflow):
    ; Call the real Entry* function.
    ; x0 = function, x1 = callInfo still in registers (unchanged).
    ; Stack has the contiguous frame for DarwinPCS va_list.
    blr     x16

    ; Restore stack and return (result is in x0)
    mov     sp, fp
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 16
    ret

NESTED_END arm64_CallDirectVarargs

;-------------------------------------------------------------------------------------------------------
; arm64_DebugTrampoline: Universal JIT->C++ call trampoline for Apple ARM64 (DarwinPCS).
;
; On DarwinPCS, variadic C++ functions read args from the stack via va_start,
; not from registers. But the JIT calling convention passes args in x0-x7.
; This trampoline bridges the gap by:
;   1. Building a contiguous stack frame: [function, callInfo, arg0, arg1, ...]
;   2. Copying any overflow args from the JIT's outgoing area
;   3. Calling the real target with both registers AND stack set up correctly
;
; Entry:
;   x16    = real call target
;   x0     = function object
;   x1     = callInfo (Count in lower 24 bits)
;   x2-x7  = arg0..arg5 (JS values)
;   LR     = return address (back into JIT code)
;   [SP+0..] = overflow args from JIT (arg6, arg7, ...) if Count > 8
;
; Stack frame built for callee:
;   [SP+0]  = function   [SP+8]  = callInfo
;   [SP+16] = arg0       [SP+24] = arg1
;   [SP+32] = arg2       [SP+40] = arg3
;   [SP+48] = arg4       [SP+56] = arg5
;   [SP+64] = arg6       ... (overflow from JIT's stack)
;
NESTED_ENTRY arm64_DebugTrampoline, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -16    ; save FP/LR, FP = SP

    ; === DEBUG: dump registers and stack before doing anything ===
    ; Save all arg regs + x16 to a temp area for the dump
    sub     sp, sp, #96             ; 10 slots * 8 + padding
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    str     x16,    [sp, #64]
    add     x9, fp, #16             ; caller's SP
    str     x9,     [sp, #72]
    mov     x0, sp                  ; pass pointer to save area
    bl      C_FUNC(debug_dump_jit_call)
    ; restore regs
    ldp     x0, x1, [sp, #0]
    ldp     x2, x3, [sp, #16]
    ldp     x4, x5, [sp, #32]
    ldp     x6, x7, [sp, #48]
    ldr     x16,    [sp, #64]
    add     sp, sp, #96
    ; === END DEBUG ===

    ; Extract JS argument count from callInfo (lower 24 bits)
    ; Count includes 'this', so total slots = Count + 2 (function + callInfo + Count args)
    ; But the JIT convention uses slots 0=function, 1=callInfo, 2+=args
    ; so x2-x7 carry args[0..5] and overflow has args[6+]
    and     x9, x1, #0xFFFFFF      ; x9 = Count

    ; Total stack slots = Count + 2 (function + callInfo + args)
    ; Minimum 8 so we can unconditionally store all register args
    add     x10, x9, #2            ; x10 = Count + 2
    cmp     x10, #8
    mov     x11, #8
    csel    x10, x10, x11, gt      ; x10 = max(Count + 2, 8)

    ; Round up to even for 16-byte alignment
    add     x10, x10, #1
    bic     x10, x10, #1

    ; Allocate the outgoing stack area
    lsl     x10, x10, #3           ; bytes
    sub     sp, sp, x10

    ; Spill all 8 register args to the stack (contiguous frame for va_list)
    stp     x0, x1, [sp, #0]       ; [SP+0]  = function, [SP+8]  = callInfo
    stp     x2, x3, [sp, #16]      ; [SP+16] = arg0,     [SP+24] = arg1
    stp     x4, x5, [sp, #32]      ; [SP+32] = arg2,     [SP+40] = arg3
    stp     x6, x7, [sp, #48]      ; [SP+48] = arg4,     [SP+56] = arg5

    ; Copy overflow args if Count > 6 (args beyond what fits in x2-x7)
    ; The JIT's overflow args are at the original SP, which is fp+16
    subs    x11, x9, #6            ; x11 = overflow count
    b.le    LOCAL_LABEL(TrampolineNoOverflow)

    add     x12, fp, #16           ; source: JIT's overflow area (original SP)
    add     x13, sp, #64           ; dest: after the 8 spilled register slots

LOCAL_LABEL(TrampolineCopyLoop):
    ldr     x14, [x12], #8
    str     x14, [x13], #8
    subs    x11, x11, #1
    b.gt    LOCAL_LABEL(TrampolineCopyLoop)

LOCAL_LABEL(TrampolineNoOverflow):
    ; Call the real target.
    ; x0-x7 are still intact (unchanged since entry).
    ; Stack now has the contiguous frame for DarwinPCS va_list.
    blr     x16

    ; Restore and return (result in x0)
    mov     sp, fp
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 16
    ret

NESTED_END arm64_DebugTrampoline

NESTED_ENTRY BreakSpeculation, _TEXT, NoHandler
    cmp x0, x0
    csel x0, x0, x0, eq
    ret
NESTED_END BreakSpeculation

;-------------------------------------------------------------------------------------------------------
; Js::InterpreterStackFrame::StaticInterpreterThunk: Entry point for JIT->Interpreter calls on Apple ARM64
;
; This replaces the C++ StaticInterpreterThunk for Darwin ARM64.
; Since we have unified the calling convention to always use a contiguous stack frame
; [function, callInfo, arg0, arg1, ...], the layout is already built on the stack
; by the caller (either arm64_CallFunction or a JIT-compiled caller).
;
; The JIT caller passes arguments in registers x0-x7 AND the full contiguous frame on stack.
; InterpreterThunk expects a JavascriptCallStackLayout* (which is just the address of 'function').
;
; Entry:
;   x0 = function object
;   x1 = callInfo
;   x2-x7 = arg0..arg5
;   [SP+0..] = overflow args (arg6, arg7, ...) if Count > 6
;
NESTED_ENTRY _ZN2Js21InterpreterStackFrame22StaticInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -16

    ; Extract arg count from callInfo (lower 24 bits)
    and     x9, x1, #0xFFFFFF      ; x9 = Count

    ; Calculate stack space needed: 16-byte aligned
    ; Space = (Count + 2) * 8, rounded up to 16
    add     x10, x9, #2            ; +2 for function, callInfo
    add     x10, x10, #1           ; round up
    lsr     x10, x10, #1           ; divide by 2
    sub     sp, sp, x10, lsl #4    ; allocate 16-byte aligned

    ; Build JavascriptCallStackLayout on stack
    stp     x0, x1, [sp, #0]       ; function, callInfo
    stp     x2, x3, [sp, #16]      ; arg0, arg1
    stp     x4, x5, [sp, #32]      ; arg2, arg3
    stp     x6, x7, [sp, #48]      ; arg4, arg5

    ; Copy overflow args (if Count > 6)
    subs    x11, x9, #6
    b.le    LOCAL_LABEL(InterpNoOverflow)

    add     x12, fp, #16           ; source: original SP (overflow from JIT)
    add     x13, sp, #64           ; dest: after 8 register slots

LOCAL_LABEL(InterpCopyLoop):
    ldr     x14, [x12], #8
    str     x14, [x13], #8
    subs    x11, x11, #1
    b.gt    LOCAL_LABEL(InterpCopyLoop)

LOCAL_LABEL(InterpNoOverflow):
    ; Call InterpreterThunk with layout pointer
    mov     x0, sp                 ; x0 = &layout
    bl      C_FUNC(_ZN2Js21InterpreterStackFrame16InterpreterThunkEPNS_25JavascriptCallStackLayoutE)

    ; Restore and return
    mov     sp, fp
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 16
    ret
NESTED_END _ZN2Js21InterpreterStackFrame22StaticInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz
