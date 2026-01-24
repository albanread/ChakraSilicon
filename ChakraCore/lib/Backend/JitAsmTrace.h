//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Backend.h"

#ifdef ENABLE_CAPSTONE_DISASM
#include <capstone/capstone.h>
#endif

namespace Js
{
    class FunctionBody;
}

//-------------------------------------------------------------------------------------------------------
// JIT Assembly Tracer
//
// This module provides comprehensive JIT assembly tracing capabilities including:
// - Real-time disassembly of generated x64 code using Capstone
// - Control flow analysis (branches, jumps, calls)
// - Register usage analysis and tracking
// - Function metadata and performance metrics
// - Assembly code formatting and output
//
// Usage:
//   JitAsmTracer tracer;
//   tracer.TraceFunction(func, codeAddress, codeSize);
//-------------------------------------------------------------------------------------------------------

class JitAsmTracer
{
public:
    // Constructor/Destructor
    JitAsmTracer();
    ~JitAsmTracer();

    // Main tracing interface
    void TraceFunction(Func* func, void* codeAddress, size_t codeSize);
    
    // Configuration
    static bool IsEnabled();
    static void SetEnabled(bool enabled);
    
    // Output control
    static void SetOutputFile(const char* filename);
    static void SetVerbosity(int level);

private:
    // Disassembly engine
    bool InitializeCapstone();
    void ShutdownCapstone();
    bool DisassembleCode(void* codeAddress, size_t codeSize, const char* functionName);
    
    // Analysis functions
    struct InstructionInfo
    {
        uint64_t address;
        size_t size;
        char mnemonic[32];
        char operands[160];
        bool isBranch;
        bool isCall;
        bool isRet;
        bool isJump;
        uint64_t target;
        int registersRead[16];  // x86_reg enum
        int registersWritten[16];
        int regReadCount;
        int regWriteCount;
    };
    
    void AnalyzeInstruction(const cs_insn* insn, InstructionInfo& info);
    void AnalyzeControlFlow(const InstructionInfo* instructions, size_t count);
    void AnalyzeRegisterUsage(const InstructionInfo* instructions, size_t count);
    void DetectHotPaths(const InstructionInfo* instructions, size_t count);
    
    // Output formatting
    void PrintFunctionHeader(const char* functionName, void* codeAddress, size_t codeSize);
    void PrintInstruction(const InstructionInfo& info, size_t index);
    void PrintControlFlowSummary(const InstructionInfo* instructions, size_t count);
    void PrintRegisterUsageSummary(const InstructionInfo* instructions, size_t count);
    void PrintPerformanceMetrics(const InstructionInfo* instructions, size_t count);
    void PrintFunctionFooter();
    
    // Function metadata extraction
    const char* GetFunctionName(Func* func);
    const char* GetSourceInfo(Func* func);
    bool GetFunctionStats(Func* func, size_t& byteCodeSize, size_t& paramCount, size_t& localCount);
    
    // Register name mapping
    const char* GetRegisterName(int reg);
    
    // Assembly analysis helpers
    bool IsVolatileRegister(int reg);
    bool IsParameterRegister(int reg);
    bool IsReturnRegister(int reg);
    int GetInstructionCost(const InstructionInfo& info);
    
    // Hot path detection
    struct BasicBlock
    {
        uint64_t startAddress;
        uint64_t endAddress;
        size_t instructionCount;
        bool isLoopHeader;
        bool isHotPath;
    };
    
    void IdentifyBasicBlocks(const InstructionInfo* instructions, size_t count, 
                           BasicBlock* blocks, size_t& blockCount);
    
    // Output management
    void WriteOutput(const char* format, ...);
    void FlushOutput();
    
private:
    // Capstone disassembler handle
#ifdef ENABLE_CAPSTONE_DISASM
    csh m_capstoneHandle;
    bool m_capstoneInitialized;
#endif
    
    // Configuration
    static bool s_enabled;
    static const char* s_outputFile;
    static int s_verbosity;
    static FILE* s_outputStream;
    
    // Statistics tracking
    static size_t s_functionsTraced;
    static size_t s_totalInstructions;
    static size_t s_totalCodeBytes;
    
    // Thread safety (simplified)
    static bool s_tracingInProgress;
    
    // Analysis state
    InstructionInfo* m_instructions;
    size_t m_instructionCapacity;
    BasicBlock* m_basicBlocks;
    size_t m_basicBlockCapacity;
    
    // Register tracking
    struct RegisterStats
    {
        int reg;
        size_t readCount;
        size_t writeCount;
        bool isHeavyUse;
    };
    
    RegisterStats m_registerStats[32]; // Enough for all x64 registers
    size_t m_registerCount;
    
    // Performance metrics
    struct FunctionMetrics
    {
        size_t instructionCount;
        size_t branchCount;
        size_t callCount;
        size_t loopCount;
        size_t estimatedCycles;
        double codeComplexity;
    };
    
    // Assembly patterns
    bool DetectLoopPattern(const InstructionInfo* instructions, size_t start, size_t count);
    bool DetectInliningPattern(const InstructionInfo* instructions, size_t count);
    bool DetectOptimizationPattern(const InstructionInfo* instructions, size_t count);
    
    // Utilities
    static const char* GetMnemonicCategory(const char* mnemonic);
    static bool IsMathInstruction(const char* mnemonic);
    static bool IsMemoryInstruction(const char* mnemonic);
    static bool IsControlFlowInstruction(const char* mnemonic);
};

//-------------------------------------------------------------------------------------------------------
// JIT Assembly Trace Scope
//
// RAII helper for automatic function tracing
//-------------------------------------------------------------------------------------------------------
class JitAsmTraceScope
{
public:
    JitAsmTraceScope(Func* func, void* codeAddress, size_t codeSize);
    ~JitAsmTraceScope();
    
private:
    JitAsmTracer m_tracer;
    bool m_active;
};

//-------------------------------------------------------------------------------------------------------
// Macros for easy integration
//-------------------------------------------------------------------------------------------------------

#define TRACE_JIT_FUNCTION(func, codeAddr, codeSize) \
    do { \
        if (JitAsmTracer::IsEnabled()) { \
            JitAsmTraceScope _traceScope(func, codeAddr, codeSize); \
        } \
    } while (0)

#define TRACE_JIT_FUNCTION_IF(condition, func, codeAddr, codeSize) \
    do { \
        if ((condition) && JitAsmTracer::IsEnabled()) { \
            JitAsmTraceScope _traceScope(func, codeAddr, codeSize); \
        } \
    } while (0)

//-------------------------------------------------------------------------------------------------------
// Configuration flags integration
//-------------------------------------------------------------------------------------------------------

// Check if JIT assembly tracing should be enabled based on command line flags
bool IsJitAsmTraceRequested();

// Initialize tracing system during engine startup
void InitializeJitAsmTracing();

// Cleanup tracing system during engine shutdown
void ShutdownJitAsmTracing();