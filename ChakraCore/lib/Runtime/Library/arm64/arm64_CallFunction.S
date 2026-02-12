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
;   arm64_CallFunction puts all args on the stack (for the stack walker) AND loads
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

NESTED_ENTRY BreakSpeculation, _TEXT, NoHandler
    cmp x0, x0
    csel x0, x0, x0, eq
    ret
NESTED_END BreakSpeculation
