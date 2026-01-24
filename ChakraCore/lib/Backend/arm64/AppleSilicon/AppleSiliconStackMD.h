//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "AppleSiliconConfig.h"

#if IS_APPLE_SILICON

//-------------------------------------------------------------------------------------------------------
// Apple Silicon Stack Management Header
//
// This header provides Apple Silicon-compatible replacements for ARM64 stack operations.
// 
// CRITICAL: Apple Silicon JIT environments prohibit STP/LDP (Store/Load Pair) instructions
// for stack manipulation. This module provides individual STR/LDR equivalents.
//
// Key transformations:
// - STP x19, x20, [sp, #offset]  ->  STR x19, [sp, #offset] + STR x20, [sp, #offset+8]
// - LDP x19, x20, [sp, #offset]  ->  LDR x19, [sp, #offset] + LDR x20, [sp, #offset+8]
//-------------------------------------------------------------------------------------------------------

namespace Js
{
    //-------------------------------------------------------------------------------------------------------
    // Apple Silicon Stack Operation Manager
    //
    // Provides safe replacements for prohibited stack pair instructions on Apple Silicon.
    // Maintains compatibility with existing ChakraCore ARM64 backend while ensuring
    // Apple Silicon JIT compliance.
    //-------------------------------------------------------------------------------------------------------
    class AppleSiliconStackManager
    {
    public:
        //-------------------------------------------------------------------------------------------------------
        // Store Operations (STP Replacements)
        //-------------------------------------------------------------------------------------------------------
        
        // Replace STP reg1, reg2, [sp, #offset] with individual STR instructions
        static void EmitStorePair_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2, 
            IR::IndirOpnd* stackLocation,
            Func* func
        );

        // Replace STP with pre-increment: STP reg1, reg2, [sp, #offset]!
        static void EmitStorePairPreIncrement_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2,
            IR::RegOpnd* stackPointer,
            int32 offset,
            Func* func
        );

        // Replace STP with post-increment: STP reg1, reg2, [sp], #offset
        static void EmitStorePairPostIncrement_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2,
            IR::RegOpnd* stackPointer,
            int32 offset,
            Func* func
        );

        //-------------------------------------------------------------------------------------------------------
        // Load Operations (LDP Replacements)
        //-------------------------------------------------------------------------------------------------------

        // Replace LDP reg1, reg2, [sp, #offset] with individual LDR instructions
        static void EmitLoadPair_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2,
            IR::IndirOpnd* stackLocation,
            Func* func
        );

        // Replace LDP with pre-increment: LDP reg1, reg2, [sp, #offset]!
        static void EmitLoadPairPreIncrement_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2,
            IR::RegOpnd* stackPointer,
            int32 offset,
            Func* func
        );

        // Replace LDP with post-increment: LDP reg1, reg2, [sp], #offset
        static void EmitLoadPairPostIncrement_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2,
            IR::RegOpnd* stackPointer,
            int32 offset,
            Func* func
        );

        //-------------------------------------------------------------------------------------------------------
        // Floating Point Operations (FSTP/FLDP Replacements)
        //-------------------------------------------------------------------------------------------------------

        // Replace FSTP (floating point store pair) with individual FSTR instructions
        static void EmitFloatStorePair_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* floatReg1,
            IR::RegOpnd* floatReg2,
            IR::IndirOpnd* stackLocation,
            Func* func
        );

        // Replace FLDP (floating point load pair) with individual FLDR instructions
        static void EmitFloatLoadPair_Individual(
            IR::Instr* insertPoint,
            IR::RegOpnd* floatReg1,
            IR::RegOpnd* floatReg2,
            IR::IndirOpnd* stackLocation,
            Func* func
        );

        //-------------------------------------------------------------------------------------------------------
        // Prolog/Epilog Specific Operations
        //-------------------------------------------------------------------------------------------------------

        // Apple Silicon-safe function prolog generation
        static void EmitFunctionProlog_AppleSilicon(
            IR::Instr* insertPoint,
            const RegList& registersToSave,
            int32 stackFrameSize,
            Func* func
        );

        // Apple Silicon-safe function epilog generation
        static void EmitFunctionEpilog_AppleSilicon(
            IR::Instr* insertPoint,
            const RegList& registersToRestore,
            int32 stackFrameSize,
            Func* func
        );

        // Save callee-saved registers using individual stores
        static void SaveCalleeRegisters_Individual(
            IR::Instr* insertPoint,
            const RegList& registers,
            IR::RegOpnd* stackPointer,
            int32 baseOffset,
            Func* func
        );

        // Restore callee-saved registers using individual loads
        static void RestoreCalleeRegisters_Individual(
            IR::Instr* insertPoint,
            const RegList& registers,
            IR::RegOpnd* stackPointer,
            int32 baseOffset,
            Func* func
        );

        //-------------------------------------------------------------------------------------------------------
        // Helper Functions
        //-------------------------------------------------------------------------------------------------------

        // Calculate stack alignment for Apple Silicon (16-byte aligned)
        static int32 CalculateAppleSiliconStackAlignment(int32 size);

        // Validate stack offset is Apple Silicon compatible
        static bool IsValidStackOffset(int32 offset);

        // Get next available stack slot aligned for Apple Silicon
        static int32 GetNextAlignedStackSlot(int32 currentOffset, int32 registerSize);

        // Convert register pair to individual register sequence
        static void ConvertPairToIndividual(
            IR::RegOpnd* reg1,
            IR::RegOpnd* reg2,
            IR::RegOpnd*& outReg1,
            IR::RegOpnd*& outReg2,
            Func* func
        );

        //-------------------------------------------------------------------------------------------------------
        // Validation and Debug Support
        //-------------------------------------------------------------------------------------------------------

        // Validate that instruction doesn't use prohibited operations
        static bool ValidateInstruction(IR::Instr* instr);

        // Check if register pair operation can be safely converted
        static bool CanConvertPairOperation(IR::Instr* instr);

        // Log Apple Silicon stack operation for debugging
        static void LogStackOperation(const char* operation, IR::Instr* instr);

        //-------------------------------------------------------------------------------------------------------
        // Configuration and State
        //-------------------------------------------------------------------------------------------------------

        // Check if Apple Silicon stack management is enabled
        static bool IsEnabled() { return IS_APPLE_SILICON; }

        // Get Apple Silicon specific stack frame requirements
        static int32 GetMinStackFrameSize();
        static int32 GetStackAlignment();
        static int32 GetRegisterSaveAreaSize();

    private:
        //-------------------------------------------------------------------------------------------------------
        // Internal Helper Methods
        //-------------------------------------------------------------------------------------------------------

        // Create individual STR instruction
        static IR::Instr* CreateStoreInstruction(
            IR::RegOpnd* sourceReg,
            IR::IndirOpnd* destLocation,
            Func* func
        );

        // Create individual LDR instruction
        static IR::Instr* CreateLoadInstruction(
            IR::RegOpnd* destReg,
            IR::IndirOpnd* sourceLocation,
            Func* func
        );

        // Calculate effective address for second register in pair
        static IR::IndirOpnd* CalculateSecondRegisterAddress(
            IR::IndirOpnd* baseLocation,
            int32 registerSize,
            Func* func
        );

        // Validate stack location for Apple Silicon constraints
        static bool ValidateStackLocation(IR::IndirOpnd* location);

        // Apply Apple Silicon specific optimizations
        static void OptimizeIndividualOperations(
            IR::Instr* instr1,
            IR::Instr* instr2,
            Func* func
        );
    };

    //-------------------------------------------------------------------------------------------------------
    // Convenience Macros for Apple Silicon Stack Operations
    //-------------------------------------------------------------------------------------------------------

    // Conditional compilation for stack pair operations
    #define EMIT_STACK_PAIR_STORE(insertPoint, reg1, reg2, location, func) \
        AppleSiliconStackManager::EmitStorePair_Individual(insertPoint, reg1, reg2, location, func)

    #define EMIT_STACK_PAIR_LOAD(insertPoint, reg1, reg2, location, func) \
        AppleSiliconStackManager::EmitLoadPair_Individual(insertPoint, reg1, reg2, location, func)

    // Function prolog/epilog macros
    #define EMIT_APPLE_SILICON_PROLOG(insertPoint, registers, frameSize, func) \
        AppleSiliconStackManager::EmitFunctionProlog_AppleSilicon(insertPoint, registers, frameSize, func)

    #define EMIT_APPLE_SILICON_EPILOG(insertPoint, registers, frameSize, func) \
        AppleSiliconStackManager::EmitFunctionEpilog_AppleSilicon(insertPoint, registers, frameSize, func)

} // namespace Js

#endif // IS_APPLE_SILICON