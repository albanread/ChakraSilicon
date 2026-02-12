//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "PrologEncoderMD.h"

//
// ARM64 prolog instruction classification for DWARF .eh_frame emission.
//
// On Windows the ARM64 prolog uses STP/LDP (paired stores/loads).
// On Apple Silicon (non-Windows), we use individual STR/LDR instructions
// in the prolog/epilog for better stability with the dynamic code unwinder.
//
// The ARM64 prolog has this general shape:
//   SUB  sp, sp, #alloc1          ; stack allocation
//   FSTR d8, [sp, #offset]        ; save callee-saved doubles (individual, non-Win)
//   FSTR d9, [sp, #offset+8]      ;   or FSTP d8, d9, [sp, #offset] (paired, Win)
//   STR  x19, [sp, #offset]       ; save callee-saved integer regs (individual, non-Win)
//   STR  x20, [sp, #offset+8]     ;   or STP x19, x20, [sp, #offset] (paired, Win)
//   STR  fp, [sp, #offset]        ; save frame pointer (individual, non-Win)
//   STR  lr, [sp, #offset+8]      ;   or STP fp, lr, [sp, #offset] (paired, Win)
//   ADD  fp, sp, #offset          ; set up frame pointer
//   SUB  sp, sp, #alloc2          ; second stack allocation (if needed)
//
// We classify these into UWOP_* codes that the PrologEncoder translates
// to DWARF CFI directives.
//

unsigned __int8 PrologEncoderMD::GetOp(IR::Instr *instr)
{
    switch (instr->m_opcode)
    {
    case Js::OpCode::SUB:
    {
        // SUB sp, sp, #imm -> stack allocation
        IR::Opnd *dst = instr->GetDst();
        IR::Opnd *src1 = instr->GetSrc1();
        if (dst && dst->IsRegOpnd() && dst->AsRegOpnd()->GetReg() == RegSP &&
            src1 && src1->IsRegOpnd() && src1->AsRegOpnd()->GetReg() == RegSP &&
            instr->GetSrc2() && instr->GetSrc2()->IsIntConstOpnd())
        {
            size_t allocSize = (size_t)instr->GetSrc2()->AsIntConstOpnd()->GetValue();
            if (allocSize <= 128)
                return UWOP_ALLOC_SMALL;
            else
                return UWOP_ALLOC_LARGE;
        }
        return UWOP_IGNORE;
    }

    case Js::OpCode::STP:
    {
        // STP reg1, reg2, [sp, #offset] -> register pair save
        // This covers both callee-saved integer pairs and fp/lr
        IR::Opnd *dst = instr->GetDst();
        if (dst && dst->IsIndirOpnd())
        {
            IR::IndirOpnd *indirDst = dst->AsIndirOpnd();
            if (indirDst->GetBaseOpnd() && indirDst->GetBaseOpnd()->GetReg() == RegSP)
            {
                return UWOP_PUSH_NONVOL;
            }
        }
        return UWOP_IGNORE;
    }

    case Js::OpCode::FSTP:
    {
        // FSTP d8, d9, [sp, #offset] -> floating-point register pair save
        IR::Opnd *dst = instr->GetDst();
        if (dst && dst->IsIndirOpnd())
        {
            IR::IndirOpnd *indirDst = dst->AsIndirOpnd();
            if (indirDst->GetBaseOpnd() && indirDst->GetBaseOpnd()->GetReg() == RegSP)
            {
                return UWOP_SAVE_XMM128;
            }
        }
        return UWOP_IGNORE;
    }

    case Js::OpCode::ADD:
    {
        // ADD fp, sp, #offset -> frame pointer setup
        // This is crucial for unwinding: emit DW_CFA_def_cfa(fp, cfa_offset - fp_offset)
        IR::Opnd *dst = instr->GetDst();
        IR::Opnd *src1 = instr->GetSrc1();
        if (dst && dst->IsRegOpnd() && dst->AsRegOpnd()->GetReg() == RegFP &&
            src1 && src1->IsRegOpnd() && src1->AsRegOpnd()->GetReg() == RegSP &&
            instr->GetSrc2() && instr->GetSrc2()->IsIntConstOpnd())
        {
            return UWOP_SET_FPREG;
        }
        return UWOP_IGNORE;
    }

    case Js::OpCode::STR:
    {
        // STR reg, [sp, #offset] -> individual register save to stack
        // On non-Windows, we use individual STR instead of STP for stability on Apple Silicon.
        IR::Opnd *dst = instr->GetDst();
        if (dst && dst->IsIndirOpnd())
        {
            IR::IndirOpnd *indirDst = dst->AsIndirOpnd();
            if (indirDst->GetBaseOpnd() && indirDst->GetBaseOpnd()->GetReg() == RegSP)
            {
                return UWOP_SAVE_NONVOL;
            }
        }
        return UWOP_IGNORE;
    }

    case Js::OpCode::FSTR:
    {
        // FSTR dreg, [sp, #offset] -> individual double register save to stack
        IR::Opnd *dst = instr->GetDst();
        if (dst && dst->IsIndirOpnd())
        {
            IR::IndirOpnd *indirDst = dst->AsIndirOpnd();
            if (indirDst->GetBaseOpnd() && indirDst->GetBaseOpnd()->GetReg() == RegSP)
            {
                return UWOP_SAVE_XMM128_FAR;
            }
        }
        return UWOP_IGNORE;
    }

    case Js::OpCode::LDP:
    case Js::OpCode::FLDP:
    case Js::OpCode::LDR:
    case Js::OpCode::RET:
    {
        // Epilog instructions -> ignore
        return UWOP_IGNORE;
    }

    default:
        // Other instructions in the prolog are not unwind-relevant
        return UWOP_IGNORE;
    }
}

size_t PrologEncoderMD::GetAllocaSize(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::SUB);
    Assert(instr->GetSrc2() && instr->GetSrc2()->IsIntConstOpnd());
    return (size_t)instr->GetSrc2()->AsIntConstOpnd()->GetValue();
}

size_t PrologEncoderMD::GetFPOffset(IR::Instr *instr)
{
    Assert(instr->m_opcode == Js::OpCode::ADD);
    Assert(instr->GetSrc2() && instr->GetSrc2()->IsIntConstOpnd());
    return (size_t)instr->GetSrc2()->AsIntConstOpnd()->GetValue();
}

