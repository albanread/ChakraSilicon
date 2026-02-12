//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//
// ARM64 prolog encoder machine-dependent helpers.
//
// This maps ARM64 prolog IR instructions to abstract unwind operations
// (UWOP_*) that the PrologEncoder can translate to DWARF CFI directives.
//

// We reuse the same UWOP codes defined in PrologEncoder.h:
//   UWOP_IGNORE      = skip this instruction
//   UWOP_PUSH_NONVOL = a register save (STP of integer reg pair)
//   UWOP_ALLOC_SMALL = a small stack allocation (SUB sp, sp, #imm where imm <= 128)
//   UWOP_ALLOC_LARGE = a large stack allocation (SUB sp, sp, #imm where imm > 128)
//   UWOP_SAVE_XMM128 = a floating-point register save (FSTP of double pair)
//   UWOP_SET_FPREG   = frame pointer setup (ADD fp, sp, #offset)
//
// On ARM64, the prolog consists of:
//   SUB sp, sp, #alloc     -> UWOP_ALLOC_SMALL or UWOP_ALLOC_LARGE
//   FSTP d8, d9, [sp, #N]  -> UWOP_SAVE_XMM128 (FP callee-saved pair)
//   STP x19, x20, [sp, #N] -> UWOP_PUSH_NONVOL (integer callee-saved pair)
//   STP fp, lr, [sp, #N]   -> UWOP_PUSH_NONVOL (frame pointer + link register)
//   ADD fp, sp, #N         -> UWOP_SET_FPREG (frame pointer setup)
//

class PrologEncoderMD
{
public:
    static unsigned __int8 GetOp(IR::Instr *instr);
    static size_t          GetAllocaSize(IR::Instr *instr);
    static size_t          GetFPOffset(IR::Instr *instr);
};

