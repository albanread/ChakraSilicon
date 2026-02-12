//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "PrologEncoderMD.h"

#ifdef _WIN32
// ----------------------------------------------------------------------------
//  _WIN32 x64 unwind uses PDATA
// ----------------------------------------------------------------------------

void PrologEncoder::RecordNonVolRegSave()
{
    requiredUnwindCodeNodeCount++;
}

void PrologEncoder::RecordXmmRegSave()
{
    requiredUnwindCodeNodeCount += 2;
}

void PrologEncoder::RecordAlloca(size_t size)
{
    Assert(size);

    requiredUnwindCodeNodeCount += PrologEncoderMD::GetRequiredNodeCountForAlloca(size);
}

DWORD PrologEncoder::SizeOfPData()
{
    return sizeof(PData) + (sizeof(UNWIND_CODE) * requiredUnwindCodeNodeCount);
}

void PrologEncoder::EncodeSmallProlog(uint8 prologSize, size_t allocaSize)
{
    Assert(allocaSize <= 128);
    Assert(requiredUnwindCodeNodeCount == 0);

    // Increment requiredUnwindCodeNodeCount to ensure we do the Alloc for the correct size
    currentUnwindCodeNodeIndex = ++requiredUnwindCodeNodeCount;

    currentInstrOffset = prologSize;

    UnwindCode* unwindCode = GetUnwindCode(1);
    size_t slots = (allocaSize - MachPtr) / MachPtr;
    uint8 unwindCodeOpInfo = TO_UNIBBLE(slots);

    unwindCode->SetOffset(prologSize);
    unwindCode->SetOp(TO_UNIBBLE(UWOP_ALLOC_SMALL));
    unwindCode->SetOpInfo(unwindCodeOpInfo);
}

void PrologEncoder::EncodeInstr(IR::Instr *instr, unsigned __int8 size)
{
    Assert(instr);
    Assert(size);

    UnwindCode       *unwindCode       = nullptr;
    unsigned __int8   unwindCodeOp     = PrologEncoderMD::GetOp(instr);
    unsigned __int8   unwindCodeOpInfo = 0;
    unsigned __int16  uint16Val        = 0;
    unsigned __int32  uint32Val        = 0;

    if (!currentInstrOffset)
        currentUnwindCodeNodeIndex = requiredUnwindCodeNodeCount;

    Assert((currentInstrOffset + size) > currentInstrOffset);
    currentInstrOffset += size;

    switch (unwindCodeOp)
    {
    case UWOP_PUSH_NONVOL:
    {
        unwindCode = GetUnwindCode(1);
        unwindCodeOpInfo = PrologEncoderMD::GetNonVolRegToSave(instr);
        break;
    }

    case UWOP_SAVE_XMM128:
    {
        unwindCode       = GetUnwindCode(2);
        unwindCodeOpInfo = PrologEncoderMD::GetXmmRegToSave(instr, &uint16Val);

        *((unsigned __int16 *)&((UNWIND_CODE *)unwindCode)[1]) = uint16Val;
        break;
    }

    case UWOP_ALLOC_SMALL:
    {
        unwindCode = GetUnwindCode(1);
        size_t allocaSize = PrologEncoderMD::GetAllocaSize(instr);
        Assert((allocaSize - MachPtr) % MachPtr == 0);
        size_t slots = (allocaSize - MachPtr) / MachPtr;
        Assert(IS_UNIBBLE(slots));
        unwindCodeOpInfo = TO_UNIBBLE(slots);
        break;
    }

    case UWOP_ALLOC_LARGE:
    {
        size_t allocaSize = PrologEncoderMD::GetAllocaSize(instr);
        Assert(allocaSize > 0x80);
        Assert(allocaSize % 8 == 0);
        Assert(IS_UNIBBLE(unwindCodeOp));

        size_t slots = allocaSize / MachPtr;
        if (allocaSize > 0x7FF8)
        {
            unwindCode       = GetUnwindCode(3);
            uint32Val        = TO_UINT32(allocaSize);
            unwindCodeOpInfo = 1;

            unwindCode->SetOp(TO_UNIBBLE(unwindCodeOp));
            *((unsigned __int32 *)&((UNWIND_CODE *)unwindCode)[1]) = uint32Val;
        }
        else
        {
            unwindCode       = GetUnwindCode(2);
            uint16Val        = TO_UINT16(slots);
            unwindCodeOpInfo = 0;

            unwindCode->SetOp(TO_UNIBBLE(unwindCodeOp));
            *((unsigned __int16 *)&((UNWIND_CODE *)unwindCode)[1]) = uint16Val;
        }
        break;
    }

    case UWOP_IGNORE:
    {
        return;
    }

    default:
    {
        AssertMsg(false, "PrologEncoderMD returned unsupported UnwindCodeOp.");
    }
    }

    Assert(unwindCode);

    unwindCode->SetOffset(currentInstrOffset);

    Assert(IS_UNIBBLE(unwindCodeOp));
    unwindCode->SetOp(TO_UNIBBLE(unwindCodeOp));

    Assert(IS_UNIBBLE(unwindCodeOpInfo));
    unwindCode->SetOpInfo(TO_UNIBBLE(unwindCodeOpInfo));
}

BYTE *PrologEncoder::Finalize(BYTE *functionStart,
                              DWORD codeSize,
                              BYTE *pdataBuffer)
{
    Assert(pdataBuffer > functionStart);
    Assert((size_t)pdataBuffer % sizeof(DWORD) == 0);

    pdata.runtimeFunction.BeginAddress = 0;
    pdata.runtimeFunction.EndAddress   = codeSize;
    pdata.runtimeFunction.UnwindData   = (DWORD)((pdataBuffer + sizeof(RUNTIME_FUNCTION)) - functionStart);

    FinalizeUnwindInfo(functionStart, codeSize);

    return (BYTE *)&pdata.runtimeFunction;
}

void PrologEncoder::FinalizeUnwindInfo(BYTE *functionStart, DWORD codeSize)
{
    pdata.unwindInfo.Version           = 1;
    pdata.unwindInfo.Flags             = 0;
    pdata.unwindInfo.SizeOfProlog      = currentInstrOffset;
    pdata.unwindInfo.CountOfCodes      = requiredUnwindCodeNodeCount;

    // We don't use the frame pointer in the standard way, and since we don't do dynamic stack allocation, we don't change the
    // stack pointer except during calls. From the perspective of the unwind info, it needs to restore information relative to
    // the stack pointer, so don't register the frame pointer.
    pdata.unwindInfo.FrameRegister     = 0;
    pdata.unwindInfo.FrameOffset       = 0;

    AssertMsg(requiredUnwindCodeNodeCount <= MaxRequiredUnwindCodeNodeCount, "We allocate 72 bytes for xdata - 34 (UnwindCodes) * 2 + 4 (UnwindInfo)");
}

PrologEncoder::UnwindCode *PrologEncoder::GetUnwindCode(unsigned __int8 nodeCount)
{
    Assert(nodeCount && ((currentUnwindCodeNodeIndex - nodeCount) >= 0));
    currentUnwindCodeNodeIndex -= nodeCount;

    return static_cast<UnwindCode *>(&pdata.unwindInfo.unwindCodes[currentUnwindCodeNodeIndex]);
}

DWORD PrologEncoder::SizeOfUnwindInfo()
{
    return (sizeof(UNWIND_INFO) + (sizeof(UNWIND_CODE) * requiredUnwindCodeNodeCount));
}

BYTE *PrologEncoder::GetUnwindInfo()
{
    return (BYTE *)&pdata.unwindInfo;
}

#else  // !_WIN32
// ----------------------------------------------------------------------------
//  !_WIN32 x64/ARM64 unwind uses .eh_frame
// ----------------------------------------------------------------------------

void PrologEncoder::EncodeSmallProlog(uint8 prologSize, size_t size)
{
    auto fde = ehFrame.GetFDE();

#if defined(_M_X64)
    // prolog: push rbp
    fde->cfi_advance_loc(1);                    // DW_CFA_advance_loc: 1
    fde->cfi_def_cfa_offset(MachPtr * 2);       // DW_CFA_def_cfa_offset: 16
    fde->cfi_offset(GetDwarfRegNum(LowererMDArch::GetRegFramePointer()), 2); // DW_CFA_offset: r6 (rbp) at cfa-16
#elif defined(_M_ARM64)
    // ARM64 small prolog:
    //   stp fp, lr, [sp, #-16]!   (4 bytes)
    //   mov fp, sp                (4 bytes)
    //
    // After stp: SP decremented by 16, CFA = SP + 16
    //   fp at [SP+0] = CFA-16, lr at [SP+8] = CFA-8
    // After mov fp, sp: FP = SP, CFA = FP + 16

    // Advance past stp (4 bytes, code_alignment_factor=1)
    fde->cfi_advance(4);
    fde->cfi_def_cfa_offset(16);                                // CFA = SP + 16
    fde->cfi_offset_auto(29, ULEB128(2));                       // FP (x29) at CFA-16 (factored: 16/8=2)
    fde->cfi_offset_auto(30, ULEB128(1));                       // LR (x30) at CFA-8  (factored: 8/8=1)

    // Advance past mov fp, sp (4 bytes)
    fde->cfi_advance(4);
    fde->cfi_def_cfa_register(ULEB128(29));                     // CFA = FP + 16 (register changes to FP, offset stays 16)
#endif

    ehFrame.End();
}

DWORD PrologEncoder::SizeOfPData()
{
    return ehFrame.Count();
}

BYTE* PrologEncoder::Finalize(BYTE *functionStart, DWORD codeSize, BYTE *pdataBuffer)
{
    auto fde = ehFrame.GetFDE();
    fde->UpdateAddressRange(functionStart, codeSize);
    return ehFrame.Buffer();
}

void PrologEncoder::Begin(size_t prologStartOffset)
{
    Assert(currentInstrOffset == 0);

    currentInstrOffset = prologStartOffset;
}

void PrologEncoder::End()
{
    ehFrame.End();
}

void PrologEncoder::FinalizeUnwindInfo(BYTE *functionStart, DWORD codeSize)
{
    auto fde = ehFrame.GetFDE();
    fde->UpdateAddressRange(functionStart, codeSize);
}

void PrologEncoder::EncodeInstr(IR::Instr *instr, unsigned __int8 size)
{
    auto fde = ehFrame.GetFDE();

    uint8 unwindCodeOp = PrologEncoderMD::GetOp(instr);

    Assert((currentInstrOffset + size) > currentInstrOffset);
    currentInstrOffset += size;

    switch (unwindCodeOp)
    {
    case UWOP_PUSH_NONVOL:
    {
#if defined(_M_X64)
        // x64: PUSH reg grows the stack by one slot
        const uword advance = currentInstrOffset - cfiInstrOffset;
        cfiInstrOffset = currentInstrOffset;
        cfaWordOffset++;

        fde->cfi_advance(advance);                              // DW_CFA_advance_loc: ?
        fde->cfi_def_cfa_offset(cfaWordOffset * MachPtr);       // DW_CFA_def_cfa_offset: ??

        const ubyte reg = PrologEncoderMD::GetNonVolRegToSave(instr) + 1;
        fde->cfi_offset(GetDwarfRegNum(reg), cfaWordOffset);    // DW_CFA_offset: r? at cfa-??
#elif defined(_M_ARM64)
        // ARM64: STP reg1, reg2, [sp, #offset]
        // The stack was already allocated by SUB sp, sp, #N, so cfaWordOffset is already set.
        // We just record where the registers are saved relative to CFA.
        const uword advance = currentInstrOffset - cfiInstrOffset;
        if (advance > 0)
        {
            fde->cfi_advance(advance);
            cfiInstrOffset = currentInstrOffset;
        }

        // Get the two source register operands and the destination offset
        IR::Opnd *src1 = instr->GetSrc1();
        IR::Opnd *src2 = instr->GetSrc2();
        IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();
        int32 offsetFromSP = dst->GetOffset();

        Assert(src1 && src1->IsRegOpnd());
        Assert(src2 && src2->IsRegOpnd());

        RegNum reg1 = src1->AsRegOpnd()->GetReg();
        RegNum reg2 = src2->AsRegOpnd()->GetReg();

        // Offset from CFA to save location: CFA = SP + cfaWordOffset*8
        // Register is at SP + offsetFromSP, so at CFA - (cfaWordOffset*8 - offsetFromSP)
        // In DWARF factored form: factored_offset = (cfaWordOffset*8 - offsetFromSP) / data_alignment
        // data_alignment = -MachPtr = -8, so factored_offset = (cfaWordOffset*8 - offsetFromSP) / 8
        Assert((size_t)(cfaWordOffset * MachPtr) >= (size_t)offsetFromSP);
        size_t cfa_minus_reg1 = cfaWordOffset * MachPtr - offsetFromSP;
        size_t cfa_minus_reg2 = cfa_minus_reg1 - MachPtr;

        // factored offset for cfi_offset: offset from CFA divided by |data_alignment_factor|
        // data_alignment_factor = -8, so factored_offset = (CFA - reg_location) / 8
        Assert(cfa_minus_reg1 % MachPtr == 0);
        Assert(cfa_minus_reg2 % MachPtr == 0);

        fde->cfi_offset_auto(GetDwarfRegNum(reg1), (ubyte)(cfa_minus_reg1 / MachPtr));
        fde->cfi_offset_auto(GetDwarfRegNum(reg2), (ubyte)(cfa_minus_reg2 / MachPtr));
#endif
        break;
    }

    case UWOP_SAVE_XMM128:
    {
#if defined(_M_ARM64)
        // ARM64: FSTP d_n, d_n+1, [sp, #offset]
        // Record where the double registers are saved relative to CFA.
        const uword advance = currentInstrOffset - cfiInstrOffset;
        if (advance > 0)
        {
            fde->cfi_advance(advance);
            cfiInstrOffset = currentInstrOffset;
        }

        IR::Opnd *src1 = instr->GetSrc1();
        IR::Opnd *src2 = instr->GetSrc2();
        IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();
        int32 offsetFromSP = dst->GetOffset();

        Assert(src1 && src1->IsRegOpnd());
        Assert(src2 && src2->IsRegOpnd());

        RegNum reg1 = src1->AsRegOpnd()->GetReg();
        RegNum reg2 = src2->AsRegOpnd()->GetReg();

        Assert((size_t)(cfaWordOffset * MachPtr) >= (size_t)offsetFromSP);
        size_t cfa_minus_reg1 = cfaWordOffset * MachPtr - offsetFromSP;
        size_t cfa_minus_reg2 = cfa_minus_reg1 - MachRegDouble;

        Assert(cfa_minus_reg1 % MachPtr == 0);
        Assert(cfa_minus_reg2 % MachPtr == 0);

        fde->cfi_offset_auto(GetDwarfRegNum(reg1), (ubyte)(cfa_minus_reg1 / MachPtr));
        fde->cfi_offset_auto(GetDwarfRegNum(reg2), (ubyte)(cfa_minus_reg2 / MachPtr));
#else
        // x64: TODO
#endif
        break;
    }

    case UWOP_ALLOC_SMALL:
    case UWOP_ALLOC_LARGE:
    {
        size_t allocaSize = PrologEncoderMD::GetAllocaSize(instr);
        Assert(allocaSize % MachPtr == 0);

        size_t slots = allocaSize / MachPtr;
        Assert(cfaWordOffset + slots > cfaWordOffset);

        const uword advance = currentInstrOffset - cfiInstrOffset;
        cfiInstrOffset = currentInstrOffset;
        cfaWordOffset += slots;

        fde->cfi_advance(advance);                          // DW_CFA_advance_loc: ?
        fde->cfi_def_cfa_offset(cfaWordOffset * MachPtr);   // DW_CFA_def_cfa_offset: ??
        break;
    }

#if defined(_M_ARM64)
    case UWOP_SET_FPREG:
    {
        // ADD fp, sp, #offset -> CFA is now FP-based
        // FP = SP + fpOffset, and CFA was SP + cfaWordOffset*8
        // So CFA = FP + (cfaWordOffset*8 - fpOffset)
        size_t fpOffset = PrologEncoderMD::GetFPOffset(instr);

        const uword advance = currentInstrOffset - cfiInstrOffset;
        if (advance > 0)
        {
            fde->cfi_advance(advance);
            cfiInstrOffset = currentInstrOffset;
        }

        size_t cfaOffsetFromFP = cfaWordOffset * MachPtr - fpOffset;
        // Use def_cfa to set both register and offset in one operation
        fde->cfi_def_cfa(ULEB128(29), ULEB128(cfaOffsetFromFP));   // DW_CFA_def_cfa: r29 (fp) ofs
        break;
    }

    case UWOP_SAVE_NONVOL:
    {
        // ARM64: STR reg, [sp, #offset] — individual integer register save
        // Used on Apple Silicon instead of STP for stability with dynamic code.
        const uword advance = currentInstrOffset - cfiInstrOffset;
        if (advance > 0)
        {
            fde->cfi_advance(advance);
            cfiInstrOffset = currentInstrOffset;
        }

        IR::Opnd *src1 = instr->GetSrc1();
        IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();
        int32 offsetFromSP = dst->GetOffset();

        Assert(src1 && src1->IsRegOpnd());
        RegNum reg = src1->AsRegOpnd()->GetReg();

        Assert((size_t)(cfaWordOffset * MachPtr) >= (size_t)offsetFromSP);
        size_t cfa_minus_reg = cfaWordOffset * MachPtr - offsetFromSP;
        Assert(cfa_minus_reg % MachPtr == 0);

        fde->cfi_offset_auto(GetDwarfRegNum(reg), (ubyte)(cfa_minus_reg / MachPtr));
        break;
    }

    case UWOP_SAVE_XMM128_FAR:
    {
        // ARM64: FSTR dreg, [sp, #offset] — individual double register save
        // Used on Apple Silicon instead of FSTP for stability with dynamic code.
        const uword advance = currentInstrOffset - cfiInstrOffset;
        if (advance > 0)
        {
            fde->cfi_advance(advance);
            cfiInstrOffset = currentInstrOffset;
        }

        IR::Opnd *src1 = instr->GetSrc1();
        IR::IndirOpnd *dst = instr->GetDst()->AsIndirOpnd();
        int32 offsetFromSP = dst->GetOffset();

        Assert(src1 && src1->IsRegOpnd());
        RegNum reg = src1->AsRegOpnd()->GetReg();

        Assert((size_t)(cfaWordOffset * MachPtr) >= (size_t)offsetFromSP);
        size_t cfa_minus_reg = cfaWordOffset * MachPtr - offsetFromSP;
        Assert(cfa_minus_reg % MachPtr == 0);

        fde->cfi_offset_auto(GetDwarfRegNum(reg), (ubyte)(cfa_minus_reg / MachPtr));
        break;
    }
#endif

    case UWOP_IGNORE:
    {
        return;
    }

    default:
    {
        AssertMsg(false, "PrologEncoderMD returned unsupported UnwindCodeOp.");
    }
    }
}

#endif  // !_WIN32
