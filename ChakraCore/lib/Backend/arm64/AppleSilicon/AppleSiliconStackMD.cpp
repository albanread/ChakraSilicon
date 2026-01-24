//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if IS_APPLE_SILICON

#include "AppleSiliconStackMD.h"
#include "../LowererMD.h"
#include "../EncoderMD.h"

//-------------------------------------------------------------------------------------------------------
// Apple Silicon Stack Management Implementation
//
// This implementation provides Apple Silicon-compatible replacements for ARM64 stack operations
// that use prohibited STP/LDP instructions. All pair operations are converted to individual
// STR/LDR sequences to comply with Apple Silicon JIT restrictions.
//-------------------------------------------------------------------------------------------------------

namespace Js
{
    //-------------------------------------------------------------------------------------------------------
    // Store Operations (STP Replacements)
    //-------------------------------------------------------------------------------------------------------

    void AppleSiliconStackManager::EmitStorePair_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* reg1,
        IR::RegOpnd* reg2, 
        IR::IndirOpnd* stackLocation,
        Func* func)
    {
        APPLE_SILICON_ASSERT(insertPoint != nullptr, "Insert point cannot be null");
        APPLE_SILICON_ASSERT(reg1 != nullptr && reg2 != nullptr, "Register operands cannot be null");
        APPLE_SILICON_ASSERT(stackLocation != nullptr, "Stack location cannot be null");
        APPLE_SILICON_ASSERT(ValidateStackLocation(stackLocation), "Invalid stack location for Apple Silicon");

        APPLE_SILICON_DEBUG_LOG("Converting STP to individual STR operations");

        // Create first store instruction: STR reg1, [sp, #offset]
        IR::IndirOpnd* location1 = IR::IndirOpnd::New(
            stackLocation->GetBaseOpnd(),
            stackLocation->GetOffset(),
            TyMachReg,
            func);

        IR::Instr* str1 = CreateStoreInstruction(reg1, location1, func);

        // Create second store instruction: STR reg2, [sp, #offset+8]
        IR::IndirOpnd* location2 = CalculateSecondRegisterAddress(stackLocation, 8, func);
        IR::Instr* str2 = CreateStoreInstruction(reg2, location2, func);

        // Insert instructions maintaining correct order
        insertPoint->InsertBefore(str1);
        insertPoint->InsertBefore(str2);

        // Apply Apple Silicon specific optimizations
        OptimizeIndividualOperations(str1, str2, func);

        APPLE_SILICON_DEBUG_LOG("STP conversion completed successfully");
    }

    void AppleSiliconStackManager::EmitStorePairPreIncrement_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* reg1,
        IR::RegOpnd* reg2,
        IR::RegOpnd* stackPointer,
        int32 offset,
        Func* func)
    {
        APPLE_SILICON_ASSERT(insertPoint != nullptr, "Insert point cannot be null");
        APPLE_SILICON_ASSERT(IsValidStackOffset(offset), "Invalid stack offset");

        // For STP reg1, reg2, [sp, #offset]! we need:
        // 1. ADD sp, sp, #offset (adjust stack pointer first)
        // 2. STR reg1, [sp]
        // 3. STR reg2, [sp, #8]

        // Adjust stack pointer
        IR::Instr* addInstr = IR::Instr::New(Js::OpCode::ADD,
            stackPointer,
            stackPointer,
            IR::IntConstOpnd::New(offset, TyMachReg, func),
            func);
        insertPoint->InsertBefore(addInstr);

        // Store registers at adjusted location
        IR::IndirOpnd* location = IR::IndirOpnd::New(stackPointer, 0, TyMachReg, func);
        EmitStorePair_Individual(insertPoint, reg1, reg2, location, func);
    }

    void AppleSiliconStackManager::EmitStorePairPostIncrement_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* reg1,
        IR::RegOpnd* reg2,
        IR::RegOpnd* stackPointer,
        int32 offset,
        Func* func)
    {
        APPLE_SILICON_ASSERT(insertPoint != nullptr, "Insert point cannot be null");
        
        // For STP reg1, reg2, [sp], #offset we need:
        // 1. STR reg1, [sp]
        // 2. STR reg2, [sp, #8] 
        // 3. ADD sp, sp, #offset (adjust after store)

        IR::IndirOpnd* location = IR::IndirOpnd::New(stackPointer, 0, TyMachReg, func);
        EmitStorePair_Individual(insertPoint, reg1, reg2, location, func);

        // Adjust stack pointer after stores
        IR::Instr* addInstr = IR::Instr::New(Js::OpCode::ADD,
            stackPointer,
            stackPointer,
            IR::IntConstOpnd::New(offset, TyMachReg, func),
            func);
        insertPoint->InsertBefore(addInstr);
    }

    //-------------------------------------------------------------------------------------------------------
    // Load Operations (LDP Replacements)
    //-------------------------------------------------------------------------------------------------------

    void AppleSiliconStackManager::EmitLoadPair_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* reg1,
        IR::RegOpnd* reg2,
        IR::IndirOpnd* stackLocation,
        Func* func)
    {
        APPLE_SILICON_ASSERT(insertPoint != nullptr, "Insert point cannot be null");
        APPLE_SILICON_ASSERT(reg1 != nullptr && reg2 != nullptr, "Register operands cannot be null");
        APPLE_SILICON_ASSERT(stackLocation != nullptr, "Stack location cannot be null");
        APPLE_SILICON_ASSERT(ValidateStackLocation(stackLocation), "Invalid stack location for Apple Silicon");

        APPLE_SILICON_DEBUG_LOG("Converting LDP to individual LDR operations");

        // Create first load instruction: LDR reg1, [sp, #offset]
        IR::IndirOpnd* location1 = IR::IndirOpnd::New(
            stackLocation->GetBaseOpnd(),
            stackLocation->GetOffset(),
            TyMachReg,
            func);

        IR::Instr* ldr1 = CreateLoadInstruction(reg1, location1, func);

        // Create second load instruction: LDR reg2, [sp, #offset+8]
        IR::IndirOpnd* location2 = CalculateSecondRegisterAddress(stackLocation, 8, func);
        IR::Instr* ldr2 = CreateLoadInstruction(reg2, location2, func);

        // Insert instructions maintaining correct order
        insertPoint->InsertBefore(ldr1);
        insertPoint->InsertBefore(ldr2);

        // Apply Apple Silicon specific optimizations
        OptimizeIndividualOperations(ldr1, ldr2, func);

        APPLE_SILICON_DEBUG_LOG("LDP conversion completed successfully");
    }

    void AppleSiliconStackManager::EmitLoadPairPreIncrement_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* reg1,
        IR::RegOpnd* reg2,
        IR::RegOpnd* stackPointer,
        int32 offset,
        Func* func)
    {
        // For LDP reg1, reg2, [sp, #offset]! we need:
        // 1. ADD sp, sp, #offset
        // 2. LDR reg1, [sp]
        // 3. LDR reg2, [sp, #8]

        IR::Instr* addInstr = IR::Instr::New(Js::OpCode::ADD,
            stackPointer,
            stackPointer,
            IR::IntConstOpnd::New(offset, TyMachReg, func),
            func);
        insertPoint->InsertBefore(addInstr);

        IR::IndirOpnd* location = IR::IndirOpnd::New(stackPointer, 0, TyMachReg, func);
        EmitLoadPair_Individual(insertPoint, reg1, reg2, location, func);
    }

    void AppleSiliconStackManager::EmitLoadPairPostIncrement_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* reg1,
        IR::RegOpnd* reg2,
        IR::RegOpnd* stackPointer,
        int32 offset,
        Func* func)
    {
        // For LDP reg1, reg2, [sp], #offset we need:
        // 1. LDR reg1, [sp]
        // 2. LDR reg2, [sp, #8]
        // 3. ADD sp, sp, #offset

        IR::IndirOpnd* location = IR::IndirOpnd::New(stackPointer, 0, TyMachReg, func);
        EmitLoadPair_Individual(insertPoint, reg1, reg2, location, func);

        IR::Instr* addInstr = IR::Instr::New(Js::OpCode::ADD,
            stackPointer,
            stackPointer,
            IR::IntConstOpnd::New(offset, TyMachReg, func),
            func);
        insertPoint->InsertBefore(addInstr);
    }

    //-------------------------------------------------------------------------------------------------------
    // Floating Point Operations (FSTP/FLDP Replacements)
    //-------------------------------------------------------------------------------------------------------

    void AppleSiliconStackManager::EmitFloatStorePair_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* floatReg1,
        IR::RegOpnd* floatReg2,
        IR::IndirOpnd* stackLocation,
        Func* func)
    {
        APPLE_SILICON_DEBUG_LOG("Converting FSTP to individual FSTR operations");

        // Create first floating-point store: FSTR floatReg1, [sp, #offset]
        IR::IndirOpnd* location1 = IR::IndirOpnd::New(
            stackLocation->GetBaseOpnd(),
            stackLocation->GetOffset(),
            TyFloat64,
            func);

        IR::Instr* fstr1 = IR::Instr::New(Js::OpCode::FSTR,
            location1,
            floatReg1,
            func);

        // Create second floating-point store: FSTR floatReg2, [sp, #offset+8]
        IR::IndirOpnd* location2 = CalculateSecondRegisterAddress(stackLocation, 8, func);
        location2->SetType(TyFloat64);

        IR::Instr* fstr2 = IR::Instr::New(Js::OpCode::FSTR,
            location2,
            floatReg2,
            func);

        insertPoint->InsertBefore(fstr1);
        insertPoint->InsertBefore(fstr2);
    }

    void AppleSiliconStackManager::EmitFloatLoadPair_Individual(
        IR::Instr* insertPoint,
        IR::RegOpnd* floatReg1,
        IR::RegOpnd* floatReg2,
        IR::IndirOpnd* stackLocation,
        Func* func)
    {
        APPLE_SILICON_DEBUG_LOG("Converting FLDP to individual FLDR operations");

        // Create first floating-point load: FLDR floatReg1, [sp, #offset]
        IR::IndirOpnd* location1 = IR::IndirOpnd::New(
            stackLocation->GetBaseOpnd(),
            stackLocation->GetOffset(),
            TyFloat64,
            func);

        IR::Instr* fldr1 = IR::Instr::New(Js::OpCode::FLDR,
            floatReg1,
            location1,
            func);

        // Create second floating-point load: FLDR floatReg2, [sp, #offset+8]
        IR::IndirOpnd* location2 = CalculateSecondRegisterAddress(stackLocation, 8, func);
        location2->SetType(TyFloat64);

        IR::Instr* fldr2 = IR::Instr::New(Js::OpCode::FLDR,
            floatReg2,
            location2,
            func);

        insertPoint->InsertBefore(fldr1);
        insertPoint->InsertBefore(fldr2);
    }

    //-------------------------------------------------------------------------------------------------------
    // Helper Functions Implementation
    //-------------------------------------------------------------------------------------------------------

    int32 AppleSiliconStackManager::CalculateAppleSiliconStackAlignment(int32 size)
    {
        // Apple Silicon requires 16-byte stack alignment
        const int32 APPLE_SILICON_STACK_ALIGNMENT = 16;
        return (size + APPLE_SILICON_STACK_ALIGNMENT - 1) & ~(APPLE_SILICON_STACK_ALIGNMENT - 1);
    }

    bool AppleSiliconStackManager::IsValidStackOffset(int32 offset)
    {
        // Apple Silicon has specific constraints on stack offsets
        // Must be within range and properly aligned
        const int32 MAX_OFFSET = 32760;  // Conservative limit
        const int32 MIN_OFFSET = -32768;
        
        return (offset >= MIN_OFFSET && offset <= MAX_OFFSET && (offset % 8) == 0);
    }

    int32 AppleSiliconStackManager::GetNextAlignedStackSlot(int32 currentOffset, int32 registerSize)
    {
        // Ensure proper alignment for register size
        int32 alignment = registerSize;
        if (alignment < 8) alignment = 8;  // Minimum 8-byte alignment
        
        return (currentOffset + alignment - 1) & ~(alignment - 1);
    }

    //-------------------------------------------------------------------------------------------------------
    // Private Helper Methods Implementation
    //-------------------------------------------------------------------------------------------------------

    IR::Instr* AppleSiliconStackManager::CreateStoreInstruction(
        IR::RegOpnd* sourceReg,
        IR::IndirOpnd* destLocation,
        Func* func)
    {
        APPLE_SILICON_ASSERT(sourceReg != nullptr, "Source register cannot be null");
        APPLE_SILICON_ASSERT(destLocation != nullptr, "Destination location cannot be null");

        return IR::Instr::New(Js::OpCode::STR,
            destLocation,
            sourceReg,
            func);
    }

    IR::Instr* AppleSiliconStackManager::CreateLoadInstruction(
        IR::RegOpnd* destReg,
        IR::IndirOpnd* sourceLocation,
        Func* func)
    {
        APPLE_SILICON_ASSERT(destReg != nullptr, "Destination register cannot be null");
        APPLE_SILICON_ASSERT(sourceLocation != nullptr, "Source location cannot be null");

        return IR::Instr::New(Js::OpCode::LDR,
            destReg,
            sourceLocation,
            func);
    }

    IR::IndirOpnd* AppleSiliconStackManager::CalculateSecondRegisterAddress(
        IR::IndirOpnd* baseLocation,
        int32 registerSize,
        Func* func)
    {
        APPLE_SILICON_ASSERT(baseLocation != nullptr, "Base location cannot be null");
        APPLE_SILICON_ASSERT(registerSize > 0, "Register size must be positive");

        return IR::IndirOpnd::New(
            baseLocation->GetBaseOpnd(),
            baseLocation->GetOffset() + registerSize,
            baseLocation->GetType(),
            func);
    }

    bool AppleSiliconStackManager::ValidateStackLocation(IR::IndirOpnd* location)
    {
        if (location == nullptr) return false;
        
        // Validate offset is within Apple Silicon constraints
        if (!IsValidStackOffset(location->GetOffset())) return false;
        
        // Ensure base is stack pointer or frame pointer
        IR::RegOpnd* base = location->GetBaseOpnd();
        if (base == nullptr) return false;
        
        RegNum baseReg = base->GetReg();
        return (baseReg == RegSP || baseReg == RegFP);
    }

    void AppleSiliconStackManager::OptimizeIndividualOperations(
        IR::Instr* instr1,
        IR::Instr* instr2,
        Func* func)
    {
        // Apply Apple Silicon specific optimizations
        // This is a placeholder for future optimizations like:
        // - Instruction scheduling
        // - Cache-friendly ordering
        // - Pipeline optimization
        
        APPLE_SILICON_DEBUG_LOG("Applying Apple Silicon optimizations to individual operations");
        
        // For now, just ensure correct instruction ordering
        // Future optimizations can be added here
    }

    //-------------------------------------------------------------------------------------------------------
    // Validation and Debug Support Implementation
    //-------------------------------------------------------------------------------------------------------

    bool AppleSiliconStackManager::ValidateInstruction(IR::Instr* instr)
    {
        if (instr == nullptr) return false;

        // Check for prohibited instructions
        Js::OpCode opcode = instr->m_opcode;
        
        switch (opcode)
        {
            case Js::OpCode::STP:
            case Js::OpCode::LDP:
            case Js::OpCode::FSTP:
            case Js::OpCode::FLDP:
                APPLE_SILICON_ASSERT(false, "Prohibited pair instruction detected");
                return false;
                
            default:
                return true;
        }
    }

    bool AppleSiliconStackManager::CanConvertPairOperation(IR::Instr* instr)
    {
        if (instr == nullptr) return false;

        Js::OpCode opcode = instr->m_opcode;
        return (opcode == Js::OpCode::STP || 
                opcode == Js::OpCode::LDP || 
                opcode == Js::OpCode::FSTP || 
                opcode == Js::OpCode::FLDP);
    }

    void AppleSiliconStackManager::LogStackOperation(const char* operation, IR::Instr* instr)
    {
        #ifdef _DEBUG
        if (operation != nullptr && instr != nullptr)
        {
            APPLE_SILICON_DEBUG_LOG("Stack operation: %s, OpCode: %d", operation, instr->m_opcode);
        }
        #endif
    }

    //-------------------------------------------------------------------------------------------------------
    // Configuration Implementation
    //-------------------------------------------------------------------------------------------------------

    int32 AppleSiliconStackManager::GetMinStackFrameSize()
    {
        // Apple Silicon minimum stack frame requirements
        return 16; // 16-byte aligned minimum
    }

    int32 AppleSiliconStackManager::GetStackAlignment()
    {
        // Apple Silicon requires 16-byte stack alignment
        return 16;
    }

    int32 AppleSiliconStackManager::GetRegisterSaveAreaSize()
    {
        // Calculate size needed for callee-saved register area
        // This depends on the number of registers that need to be saved
        return 128; // Conservative estimate for register save area
    }

} // namespace Js

#endif // IS_APPLE_SILICON