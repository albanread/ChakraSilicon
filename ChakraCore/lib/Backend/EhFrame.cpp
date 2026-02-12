//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "EhFrame.h"

#if defined(_M_X64)
// AMD64 ABI -- DWARF register number mapping
static const ubyte DWARF_RegNum[] =
{
    // Exactly same order as RegList.h!
    -1, // NOREG,
    0,  // RAX,
    2,  // RCX,
    1,  // RDX,
    3,  // RBX,
    7,  // RSP,
    6,  // RBP,
    4,  // RSI,
    5,  // RDI,
    8,  // R8,
    9,  // R9,
    10, // R10,
    11, // R11,
    12, // R12,
    13, // R13,
    14, // R14,
    15, // R15,
    17,  // XMM0,
    18,  // XMM1,
    19,  // XMM2,
    20,  // XMM3,
    21,  // XMM4,
    22,  // XMM5,
    23,  // XMM6,
    24,  // XMM7,
    25,  // XMM8,
    26,  // XMM9,
    27,  // XMM10,
    28,  // XMM11,
    29,  // XMM12,
    30,  // XMM13,
    31,  // XMM14,
    32,  // XMM15,
};

// x64: return address is implicitly pushed by CALL, DWARF register 16 = RA
static const ubyte DWARF_RegRA = 16;

#elif defined(_M_ARM64)
// ARM64 ABI -- DWARF register number mapping
// ARM64 DWARF register numbers: x0-x30 = 0-30, SP = 31, d0-d31 = 64-95
static const ubyte DWARF_RegNum[] =
{
    // Exactly same order as arm64/RegList.h!
    (ubyte)-1, // NOREG,
    0,   // R0  (x0),
    1,   // R1  (x1),
    2,   // R2  (x2),
    3,   // R3  (x3),
    4,   // R4  (x4),
    5,   // R5  (x5),
    6,   // R6  (x6),
    7,   // R7  (x7),
    8,   // R8  (x8),
    9,   // R9  (x9),
    10,  // R10 (x10),
    11,  // R11 (x11),
    12,  // R12 (x12),
    13,  // R13 (x13),
    14,  // R14 (x14),
    15,  // R15 (x15),
    16,  // R16 (x16, IP0),
    17,  // R17 (x17, IP1),
    18,  // R18 (x18, platform register),
    19,  // R19,
    20,  // R20,
    21,  // R21,
    22,  // R22,
    23,  // R23,
    24,  // R24,
    25,  // R25,
    26,  // R26,
    27,  // R27,
    28,  // R28,
    29,  // FP  (x29),
    30,  // LR  (x30),
    31,  // SP,
    31,  // ZR  (maps to SP in DWARF; ZR is not a real register for unwinding),
    // VFP/NEON double registers: DWARF 64-93
    64,  // D0,
    65,  // D1,
    66,  // D2,
    67,  // D3,
    68,  // D4,
    69,  // D5,
    70,  // D6,
    71,  // D7,
    72,  // D8,
    73,  // D9,
    74,  // D10,
    75,  // D11,
    76,  // D12,
    77,  // D13,
    78,  // D14,
    79,  // D15,
    80,  // D16,
    81,  // D17,
    82,  // D18,
    83,  // D19,
    84,  // D20,
    85,  // D21,
    86,  // D22,
    87,  // D23,
    88,  // D24,
    89,  // D25,
    90,  // D26,
    91,  // D27,
    92,  // D28,
    93,  // D29,
};

// ARM64: return address is in LR (x30), DWARF register 30
static const ubyte DWARF_RegRA = 30;

#else
#error "Unsupported architecture for EhFrame"
#endif

ubyte GetDwarfRegNum(ubyte regNum)
{
    return DWARF_RegNum[regNum];
}

// Encode into ULEB128 (Unsigned Little Endian Base 128)
BYTE* EmitLEB128(BYTE* pc, unsigned value)
{
    do
    {
        BYTE b = value & 0x7F; // low order 7 bits
        value >>= 7;

        if (value)  // more bytes to come
        {
            b |= 0x80;
        }

        *pc++ = b;
    }
    while (value != 0);

    return pc;
}

// Encode into signed LEB128 (Signed Little Endian Base 128)
BYTE* EmitLEB128(BYTE* pc, int value)
{
    static const int size = sizeof(value) * 8;
    static const bool isLogicShift = (-1 >> 1) != -1;

    const bool signExtend = isLogicShift && value < 0;

    bool more = true;
    while (more)
    {
        BYTE b = value & 0x7F; // low order 7 bits
        value >>= 7;

        if (signExtend)
        {
            value |= - (1 << (size - 7)); // sign extend
        }

        const bool signBit = (b & 0x40) != 0;
        if ((value == 0 && !signBit) || (value == -1 && signBit))
        {
            more = false;
        }
        else
        {
            b |= 0x80;
        }

        *pc++ = b;
    }

    return pc;
}


void EhFrame::Entry::Begin()
{
    Assert(beginOffset == -1);
    beginOffset = writer->Count();

    // Write Length place holder
    const uword length = 0;
    writer->Write(length);
}

void EhFrame::Entry::End()
{
    Assert(beginOffset != -1); // Must have called Begin()

    // padding
    size_t padding = (MachPtr - writer->Count() % MachPtr) % MachPtr;
    for (size_t i = 0; i < padding; i++)
    {
        cfi_nop();
    }

    // update length record
    uword length = writer->Count() - beginOffset
                    - sizeof(length);  // exclude length itself
    writer->Write(beginOffset, length);
}

void EhFrame::Entry::cfi_advance(uword advance)
{
    if (advance <= 0x3F)        // 6-bits
    {
        cfi_advance_loc(static_cast<ubyte>(advance));
    }
    else if (advance <= 0xFF)   // 1-byte
    {
        cfi_advance_loc1(static_cast<ubyte>(advance));
    }
    else if (advance <= 0xFFFF) // 2-byte
    {
        cfi_advance_loc2(static_cast<uword>(advance));
    }
    else                        // 4-byte
    {
        cfi_advance_loc4(advance);
    }
}

void EhFrame::CIE::Begin()
{
    Assert(writer->Count() == 0);
    Entry::Begin();

    const uword cie_id = 0;
    Emit(cie_id);

    const ubyte version = 1;
    Emit(version);

    // Augmentation string: "zR"
    //   z = augmentation data follows (with length prefix)
    //   R = FDE pointer encoding byte follows in augmentation data
    Emit((ubyte)'z');
    Emit((ubyte)'R');
    Emit((ubyte)0);  // null terminator

    const ULEB128 codeAlignmentFactor = 1;
    Emit(codeAlignmentFactor);

    const LEB128 dataAlignmentFactor = - MachPtr;
    Emit(dataAlignmentFactor);

    const ubyte returnAddressRegister = DWARF_RegRA;
    Emit(returnAddressRegister);

    // Augmentation data for "zR": length + FDE pointer encoding
    const ULEB128 augDataLength = 1;  // 1 byte of augmentation data
    Emit(augDataLength);
    const ubyte fdeEncoding = 0x00;   // DW_EH_PE_absptr (absolute pointer)
    Emit(fdeEncoding);
}


void EhFrame::FDE::Begin()
{
    Entry::Begin();

    const uword cie_id = writer->Count();
    Emit(cie_id);

    // Write pc <begin, range> placeholder
    pcBeginOffset = writer->Count();
    const void* pc = nullptr;
    Emit(pc);
    Emit(pc);

    // FDE augmentation data length (required when CIE has "z" augmentation)
    // We have no FDE-specific augmentation data (no LSDA pointer)
    const ULEB128 augDataLength = 0;
    Emit(augDataLength);
}

void EhFrame::FDE::UpdateAddressRange(const void* pcBegin, size_t pcRange)
{
    writer->Write(pcBeginOffset, pcBegin);
    writer->Write(pcBeginOffset + sizeof(pcBegin),
        reinterpret_cast<const void*>(pcRange));
}


EhFrame::EhFrame(BYTE* buffer, size_t size)
        : writer(buffer, size), fde(&writer)
{
    CIE cie(&writer);
    cie.Begin();

    // CIE initial instructions
#if defined(_M_X64)
    // x64: after CALL, RSP points to return address, so CFA = RSP + 8
    // DW_CFA_def_cfa: r7 (rsp) ofs 8
    // RSP is RegRSP (index 5 in RegList.h), DWARF_RegNum[5] = 7
    cie.cfi_def_cfa(DWARF_RegNum[5], MachPtr);
    // DW_CFA_offset: r16 (rip/ra) at cfa-8 (data alignment factor is -8, so factored offset = 1)
    cie.cfi_offset(DWARF_RegRA, 1);
#elif defined(_M_ARM64)
    // ARM64: SP is CFA at function entry, CFA = SP + 0 initially
    // SP is RegSP (index 32 in RegList.h after NOREG + 31 GP regs + ZR), DWARF_RegNum = 31
    // Use the DWARF SP register number directly: 31
    cie.cfi_def_cfa((ubyte)31, 0);
    // Note: on ARM64, LR is explicitly saved by prolog instructions (STP fp, lr).
    // We do NOT emit a cfi_offset for RA here - it will be emitted when LR is saved.
#endif

    cie.End();

    fde.Begin();
}

void EhFrame::End()
{
    fde.End();

    // Write length 0 to mark terminate entry
    const uword terminate_entry_length = 0;
    writer.Write(terminate_entry_length);
}
