;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

.global C_FUNC(_ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

#ifdef _ENABLE_DYNAMIC_THUNKS

.global C_FUNC(_ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

;;============================================================================================================
;; InterpreterStackFrame::DelayDynamicInterpreterThunk
;;============================================================================================================
    ;Var InterpreterStackFrame::DelayDynamicInterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...)
    ;
    ; Must save ALL argument registers x0-x7 (not just x0-x1) because:
    ;   1. The bl to EnsureDynamicInterpreterThunk clobbers volatile x2-x7.
    ;   2. The tail-jump target (dynamic interpreter thunk) needs correct x0-x7 to build
    ;      the JavascriptCallStackLayout contiguous stack frame.
    ;   3. On DarwinPCS (Apple ARM64), the JIT caller passes args in x0-x7 + overflow on
    ;      stack; if we lose x2-x7, the interpreter reads garbage for function arguments.

NESTED_ENTRY _ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -80   ; save fp/lr, allocate 80 bytes
    stp     x0, x1, [sp, #16]                  ; save function, callInfo
    stp     x2, x3, [sp, #32]                  ; save arg0, arg1
    stp     x4, x5, [sp, #48]                  ; save arg2, arg3
    stp     x6, x7, [sp, #64]                  ; save arg4, arg5

    bl      C_FUNC(_ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE)  ; call InterpreterStackFrame::EnsureDynamicInterpreterThunk
    mov     x16, x0                  ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore all argument registers
    ldp     x2, x3, [sp, #32]
    ldp     x4, x5, [sp, #48]
    ldp     x6, x7, [sp, #64]

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 80
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

;;============================================================================================================
;; DynamicProfileInfo::EnsureDynamicProfileInfoThunk
;;============================================================================================================
    ;Var DynamicProfileInfo::EnsureDynamicProfileInfoThunk(RecyclableObject* function, CallInfo callInfo, ...)
    ; Must save ALL argument registers x0-x7. See DelayDynamicInterpreterThunk comment.
NESTED_ENTRY _ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -80   ; save fp/lr, allocate 80 bytes
    stp     x0, x1, [sp, #16]                  ; save function, callInfo
    stp     x2, x3, [sp, #32]                  ; save arg0, arg1
    stp     x4, x5, [sp, #48]                  ; save arg2, arg3
    stp     x6, x7, [sp, #64]                  ; save arg4, arg5

    bl      C_FUNC(_ZN2Js18DynamicProfileInfo24EnsureDynamicProfileInfoEPNS_14ScriptFunctionE) ; call DynamicProfileInfo::EnsureDynamicProfileInfo
    mov     x16, x0                  ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore all argument registers
    ldp     x2, x3, [sp, #32]
    ldp     x4, x5, [sp, #48]
    ldp     x6, x7, [sp, #64]

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 80
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

#endif

;;============================================================================================================
;; ScriptContext::ProfileModeDeferredParsingThunk
;;============================================================================================================
    ;; Var ScriptContext::ProfileModeDeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
    ;; Must save ALL argument registers x0-x7 for correct tail-jump on DarwinPCS.
NESTED_ENTRY _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -80   ; save fp/lr, allocate 80 bytes
    stp     x0, x1, [sp, #16]                  ; save function, callInfo
    stp     x2, x3, [sp, #32]                  ; save arg0, arg1
    stp     x4, x5, [sp, #48]                  ; save arg2, arg3
    stp     x6, x7, [sp, #64]                  ; save arg4, arg5

    mov     x0, sp                   ; Pass the address of the function at the saved x0 in case it need to be boxed
    add     x0, x0, #16              ; 16 is subtracted from the stack pointer when the a function is called, add it back here.
    bl      C_FUNC(_ZN2Js13ScriptContext24ProfileModeDeferredParseEPPNS_14ScriptFunctionE) ; call ScriptContext::ProfileModeDeferredParse
    mov     x16, x0                  ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore all argument registers
    ldp     x2, x3, [sp, #32]
    ldp     x4, x5, [sp, #48]
    ldp     x6, x7, [sp, #64]

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 80
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

;;============================================================================================================
;; ScriptContext::ProfileModeDeferredDeserializeThunk
;;============================================================================================================
    ;; Var ScriptContext::ProfileModeDeferredDeserializeThunk(RecyclableObject* function, CallInfo callInfo, ...)
    ;; Must save ALL argument registers x0-x7 for correct tail-jump on DarwinPCS.
NESTED_ENTRY _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -80   ; save fp/lr, allocate 80 bytes
    stp     x0, x1, [sp, #16]                  ; save function, callInfo
    stp     x2, x3, [sp, #32]                  ; save arg0, arg1
    stp     x4, x5, [sp, #48]                  ; save arg2, arg3
    stp     x6, x7, [sp, #64]                  ; save arg4, arg5

    bl      C_FUNC(_ZN2Js13ScriptContext30ProfileModeDeferredDeserializeEPNS_14ScriptFunctionE) ; call ScriptContext::ProfileModeDeferredDeserialize
    mov     x16, x0                     ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore all argument registers
    ldp     x2, x3, [sp, #32]
    ldp     x4, x5, [sp, #48]
    ldp     x6, x7, [sp, #64]

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 80
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT
