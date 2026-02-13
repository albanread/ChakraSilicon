//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Instruction encoding macros.  Used by ARM instruction encoder.
//
#define UNUSED 0

//            opcode
//           /           layout
//          /           /          attrib          byte2
//         /           /          /               /         form
//        /           /          /               /         /               unused
//       /           /          /               /         /               /
//      /           /          /               /         /               /         dope
//     /           /          /               /         /               /         /

MACRO(ADD,        Reg3,       0,              UNUSED,   LEGAL_ADDSUB,   UNUSED,   D___)
MACRO(ADDS,       Reg3,       OpSideEffect,   UNUSED,   LEGAL_ADDSUB,   UNUSED,   D__S)
MACRO(ADR,        Reg3,       0,              UNUSED,   LEGAL_LABEL,    UNUSED,   D___)
MACRO(AND,        Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
MACRO(ANDS,       Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D__S)
MACRO(ASR,        Reg3,       0,              UNUSED,   LEGAL_SHIFT,    UNUSED,   D___)
MACRO(B,          Br,         OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BEQ,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BNE,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BLT,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BLE,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BGT,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BGE,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BCS,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BCC,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BHI,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BLS,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BMI,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BPL,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BVS,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BVC,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BFI,        Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)
MACRO(BFXIL,      Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)
MACRO(BIC,        Reg3,       OpSideEffect,   UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
MACRO(BL,         CallI,      OpSideEffect,   UNUSED,   LEGAL_CALL,     UNUSED,   D___)
MACRO(BLR,        CallI,      OpSideEffect,   UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(BR,         Br,         OpSideEffect,   UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(CBZ,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_CBZ,      UNUSED,   D___)
MACRO(CBNZ,       BrReg2,     OpSideEffect,   UNUSED,   LEGAL_CBZ,      UNUSED,   D___)
MACRO(CLZ,        Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(CMP,        Reg1,       OpSideEffect,   UNUSED,   LEGAL_PSEUDO,   UNUSED,   D__S)
MACRO(CMN,        Reg1,       OpSideEffect,   UNUSED,   LEGAL_PSEUDO,   UNUSED,   D__S)
// CMP src1, src2, SXTW -- used in multiply overflow checks
MACRO(CMP_SXTW,   Reg1,       OpSideEffect,   UNUSED,   LEGAL_REG3_ND,  UNUSED,   D__S)
// CSELcc src1, src2 -- select src1 if cc or src2 if not
MACRO(CSELEQ,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(CSELNE,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(CSELLT,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// CSNEGPL src1, src2 -- select src1 if PL or -src1 if not; used in integer absolute value
MACRO(CSNEGPL,    Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(DEBUGBREAK, Reg1,       OpSideEffect,   UNUSED,   LEGAL_NONE,     UNUSED,   D___)
MACRO(EOR,        Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
// EOR src1, src2, ASR #31/63 -- used in floating-point-to-integer overflow checks
MACRO(EOR_ASR31,  Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(LDIMM,      Reg2,       0,              UNUSED,   LEGAL_LDIMM,    UNUSED,   DM__)
MACRO(LDP,        Reg3,       0,              UNUSED,   LEGAL_LOADP,    UNUSED,   DL__)
MACRO(LDP_POST,   Reg3,       0,              UNUSED,   LEGAL_LOADP,    UNUSED,   DL__)
MACRO(LDR,        Reg2,       0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(LDRS,       Reg2,       0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(LEA,        Reg3,       0,              UNUSED,   LEGAL_LEA,      UNUSED,   D___)
MACRO(LSL,        Reg2,       0,              UNUSED,   LEGAL_SHIFT,    UNUSED,   D___)
MACRO(LSR,        Reg2,       0,              UNUSED,   LEGAL_SHIFT,    UNUSED,   D___)
MACRO(MOV,        Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
// Alias of MOV that won't get optimized out when src and dst are the same.
MACRO(MOV_TRUNC,  Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
MACRO(MOVK,       Reg2,       0,              UNUSED,   LEGAL_LDIMM_S,    UNUSED,   DM__)
MACRO(MOVN,       Reg2,       0,              UNUSED,   LEGAL_LDIMM_S,    UNUSED,   DM__)
MACRO(MOVZ,       Reg2,       0,              UNUSED,   LEGAL_LDIMM_S,    UNUSED,   DM__)
MACRO(MRS_FPCR,   Reg1,       0,              UNUSED,   LEGAL_REG1,     UNUSED,   D___)
MACRO(MRS_FPSR,   Reg1,       0,              UNUSED,   LEGAL_REG1,     UNUSED,   D___)
MACRO(MSR_FPCR,   Reg2,       0,              UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(MSR_FPSR,   Reg2,       0,              UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(MSUB,       Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(MUL,        Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(MVN,        Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(NOP,        Empty,      0,              UNUSED,   LEGAL_NONE,     UNUSED,   D___)
MACRO(ORR,        Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
MACRO(PLD,        Reg2,       0,              UNUSED,   LEGAL_PLD,      UNUSED,   DL__)
MACRO(REM,        Reg3,       OpSideEffect,   UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(RET,        Reg2,       OpSideEffect,   UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(SBFX,       Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)
MACRO(SDIV,       Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(SMADDL,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(SMULL,      Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(STP,        Reg3,       0,              UNUSED,   LEGAL_STOREP,   UNUSED,   DL__)
MACRO(STP_PRE,    Reg3,       0,              UNUSED,   LEGAL_STOREP,   UNUSED,   DL__)
MACRO(STR,        Reg2,       0,              UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
MACRO(SUB,        Reg3,       0,              UNUSED,   LEGAL_ADDSUB,   UNUSED,   D___)
// SUB dst, src1, src2 LSL #4 -- used in prologs with _chkstk calls
MACRO(SUB_LSL4,   Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(SUBS,       Reg3,       OpSideEffect,   UNUSED,   LEGAL_ADDSUB,   UNUSED,   D__S)
MACRO(TBNZ,       BrReg2,     OpSideEffect,   UNUSED,   LEGAL_TBZ,      UNUSED,   D___)
MACRO(TBZ,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_TBZ,      UNUSED,   D___)
MACRO(TST,        Reg2,       OpSideEffect,   UNUSED,   LEGAL_PSEUDO,   UNUSED,   D__S)
MACRO(UBFX,       Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)

// Pseudo-op that loads the size of the arg out area. A special op with no src is used so that the
// actual arg out size can be fixed up by the encoder.
MACRO(LDARGOUTSZ, Reg1,       0,              UNUSED,   LEGAL_REG1,     UNUSED,   D___)

//VFP instructions:
MACRO(FABS,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FADD,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FCSELcc src1, src2 -- select src1 if cc or src2 if not
MACRO(FCSELEQ,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FCSELNE,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FCMP,        Reg1,      OpSideEffect,   UNUSED,   LEGAL_REG3_ND,  UNUSED,   D___)
MACRO(FCVT,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTM,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTN,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTP,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTZ,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FDIV,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FLDR,        Reg2,      0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(FLDP,        Reg2,      0,              UNUSED,   LEGAL_LOADP,    UNUSED,   DL__)
MACRO(FMIN,        Reg2,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FMAX,        Reg2,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FMOV,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
MACRO(FMOV_GEN,    Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
MACRO(FMUL,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FNEG,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FRINTM,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FRINTP,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FSUB,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FSQRT,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FSTR,        Reg2,      0,              UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
MACRO(FSTP,        Reg2,      0,              UNUSED,   LEGAL_STOREP,   UNUSED,   DS__)

//-------------------------------------------------------------------------------------------------------
// NEON Vector Instructions (Phase 2)
//
// These opcodes use the D0-D29 float register file in 128-bit (Q/V) mode.
// The encoder maps them to the EmitNeon* functions in ARM64NeonEncoder.h.
// Register allocation reuses the existing float register pool since D and V
// registers share the same physical storage on ARM64.
//-------------------------------------------------------------------------------------------------------

// --- Data Movement ---
// DUP Vd.<T>, Rn  — broadcast general-purpose register to all vector lanes
MACRO(NEON_DUP,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// MOVI Vd.<T>, #imm — move immediate into all vector lanes
MACRO(NEON_MOVI,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// MOV Vd.<T>, Vn.<T> — vector register move (alias for ORR Vd, Vn, Vn)
MACRO(NEON_MOV,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)

// --- Load / Store ---
// LD1 {Vt.<T>}, [Xn] — load single vector from memory (no writeback)
MACRO(NEON_LD1,       Reg2,      0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
// ST1 {Vt.<T>}, [Xn] — store single vector to memory (no writeback)
MACRO(NEON_ST1,       Reg2,      0,              UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
// LDR Qt, [Xn, #imm] — 128-bit vector load with unsigned offset
MACRO(NEON_LDR_Q,     Reg2,      0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
// STR Qt, [Xn, #imm] — 128-bit vector store with unsigned offset
MACRO(NEON_STR_Q,     Reg2,      0,              UNUSED,   LEGAL_STORE,    UNUSED,   DS__)

// --- Integer Arithmetic (vector) ---
// ADD Vd.<T>, Vn.<T>, Vm.<T> — vector integer add
MACRO(NEON_ADD,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// SUB Vd.<T>, Vn.<T>, Vm.<T> — vector integer subtract
MACRO(NEON_SUB,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// MUL Vd.<T>, Vn.<T>, Vm.<T> — vector integer multiply
MACRO(NEON_MUL,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// NEG Vd.<T>, Vn.<T> — vector integer negate
MACRO(NEON_NEG,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// ABS Vd.<T>, Vn.<T> — vector integer absolute value
MACRO(NEON_ABS,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)

// --- Floating-Point Arithmetic (vector) ---
// FADD Vd.<T>, Vn.<T>, Vm.<T> — vector float add (4S or 2D)
MACRO(NEON_FADD,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FSUB Vd.<T>, Vn.<T>, Vm.<T> — vector float subtract
MACRO(NEON_FSUB,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMUL Vd.<T>, Vn.<T>, Vm.<T> — vector float multiply
MACRO(NEON_FMUL,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FDIV Vd.<T>, Vn.<T>, Vm.<T> — vector float divide
MACRO(NEON_FDIV,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FNEG Vd.<T>, Vn.<T> — vector float negate
MACRO(NEON_FNEG,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FABS Vd.<T>, Vn.<T> — vector float absolute value
MACRO(NEON_FABS,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FSQRT Vd.<T>, Vn.<T> — vector float square root
MACRO(NEON_FSQRT,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FMLA Vd.<T>, Vn.<T>, Vm.<T> — vector fused multiply-accumulate (Vd += Vn * Vm)
MACRO(NEON_FMLA,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMLS Vd.<T>, Vn.<T>, Vm.<T> — vector fused multiply-subtract (Vd -= Vn * Vm)
MACRO(NEON_FMLS,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Min / Max (vector) ---
// SMIN Vd.<T>, Vn.<T>, Vm.<T> — vector signed integer minimum
MACRO(NEON_SMIN,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// SMAX Vd.<T>, Vn.<T>, Vm.<T> — vector signed integer maximum
MACRO(NEON_SMAX,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// UMIN Vd.<T>, Vn.<T>, Vm.<T> — vector unsigned integer minimum
MACRO(NEON_UMIN,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// UMAX Vd.<T>, Vn.<T>, Vm.<T> — vector unsigned integer maximum
MACRO(NEON_UMAX,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMIN Vd.<T>, Vn.<T>, Vm.<T> — vector float minimum
MACRO(NEON_FMIN,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMAX Vd.<T>, Vn.<T>, Vm.<T> — vector float maximum
MACRO(NEON_FMAX,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMINNM Vd.<T>, Vn.<T>, Vm.<T> — vector float min (NaN-propagating)
MACRO(NEON_FMINNM,    Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMAXNM Vd.<T>, Vn.<T>, Vm.<T> — vector float max (NaN-propagating)
MACRO(NEON_FMAXNM,    Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Horizontal Reduction ---
// ADDV Sd, Vn.<T> — add across vector lanes (integer)
MACRO(NEON_ADDV,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// SMAXV Sd, Vn.<T> — signed maximum across vector lanes
MACRO(NEON_SMAXV,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// SMINV Sd, Vn.<T> — signed minimum across vector lanes
MACRO(NEON_SMINV,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FADDP Vd.<T>, Vn.<T>, Vm.<T> — float pairwise add
MACRO(NEON_FADDP,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FMAXNMV Sd, Vn.4S — float max across vector (NaN-propagating)
MACRO(NEON_FMAXNMV,   Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FMINNMV Sd, Vn.4S — float min across vector (NaN-propagating)
MACRO(NEON_FMINNMV,   Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)

// --- Comparison (vector) ---
// CMEQ Vd.<T>, Vn.<T>, Vm.<T> — compare equal
MACRO(NEON_CMEQ,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// CMGT Vd.<T>, Vn.<T>, Vm.<T> — compare signed greater-than
MACRO(NEON_CMGT,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// CMGE Vd.<T>, Vn.<T>, Vm.<T> — compare signed greater-than-or-equal
MACRO(NEON_CMGE,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// CMEQ Vd.<T>, Vn.<T>, #0 — compare equal to zero
MACRO(NEON_CMEQ0,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FCMEQ Vd.<T>, Vn.<T>, Vm.<T> — float compare equal
MACRO(NEON_FCMEQ,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FCMGT Vd.<T>, Vn.<T>, Vm.<T> — float compare greater-than
MACRO(NEON_FCMGT,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FCMGE Vd.<T>, Vn.<T>, Vm.<T> — float compare greater-than-or-equal
MACRO(NEON_FCMGE,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Bitwise Logic (vector) ---
// AND Vd.16B, Vn.16B, Vm.16B — vector bitwise AND
MACRO(NEON_AND,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// ORR Vd.16B, Vn.16B, Vm.16B — vector bitwise OR
MACRO(NEON_ORR,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// EOR Vd.16B, Vn.16B, Vm.16B — vector bitwise XOR
MACRO(NEON_EOR,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// NOT Vd.16B, Vn.16B — vector bitwise NOT
MACRO(NEON_NOT,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// BSL Vd.16B, Vn.16B, Vm.16B — bitwise select (Vd = (Vd & Vn) | (~Vd & Vm))
MACRO(NEON_BSL,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// BIC Vd.16B, Vn.16B, Vm.16B — bitwise clear (Vd = Vn & ~Vm)
MACRO(NEON_BIC,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Shift (vector) ---
// SHL Vd.<T>, Vn.<T>, #shift — vector shift left by immediate
MACRO(NEON_SHL,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// SSHR Vd.<T>, Vn.<T>, #shift — vector signed shift right by immediate
MACRO(NEON_SSHR,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// USHR Vd.<T>, Vn.<T>, #shift — vector unsigned shift right by immediate
MACRO(NEON_USHR,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Permute / Shuffle ---
// REV64 Vd.<T>, Vn.<T> — reverse elements within 64-bit doublewords
MACRO(NEON_REV64,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// REV32 Vd.<T>, Vn.<T> — reverse elements within 32-bit words
MACRO(NEON_REV32,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// REV16 Vd.<T>, Vn.<T> — reverse elements within 16-bit halfwords
MACRO(NEON_REV16,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// EXT Vd.16B, Vn.16B, Vm.16B, #idx — extract from pair of vectors
MACRO(NEON_EXT,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// TBL Vd.16B, {Vn.16B}, Vm.16B — table lookup
MACRO(NEON_TBL,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Type Conversion (vector) ---
// SCVTF Vd.<T>, Vn.<T> — vector signed int to float
MACRO(NEON_SCVTF,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// UCVTF Vd.<T>, Vn.<T> — vector unsigned int to float
MACRO(NEON_UCVTF,     Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FCVTZS Vd.<T>, Vn.<T> — vector float to signed int (round toward zero)
MACRO(NEON_FCVTZS,    Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// FCVTZU Vd.<T>, Vn.<T> — vector float to unsigned int (round toward zero)
MACRO(NEON_FCVTZU,    Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)

// --- Element Insert / Extract ---
// INS Vd.<T>[idx], Rn — insert general-purpose register into vector lane
MACRO(NEON_INS,       Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// UMOV Rd, Vn.<T>[idx] — move vector lane to general-purpose register (unsigned)
MACRO(NEON_UMOV,      Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// DUP Vd.<T>, Vn.<T>[idx] — duplicate vector element to all lanes
MACRO(NEON_DUP_ELEM,  Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)

// --- Widen / Narrow ---
// SXTL Vd.<Td>, Vn.<Ts> — signed extend long (alias for SSHLL #0)
MACRO(NEON_SXTL,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// UXTL Vd.<Td>, Vn.<Ts> — unsigned extend long (alias for USHLL #0)
MACRO(NEON_UXTL,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
// XTN Vd.<Ts>, Vn.<Td> — extract narrow
MACRO(NEON_XTN,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)

// --- Prefetch ---
// PRFM <type>, [Xn, #imm] — prefetch memory (PLDL1KEEP, PSTL1KEEP, etc.)
MACRO(NEON_PRFM,      Reg2,      0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
