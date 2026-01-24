//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"
#include "JitAsmTrace.h"

#ifdef ENABLE_CAPSTONE_DISASM
#include <capstone/capstone.h>
#endif

// External function declaration
extern "C" bool IsTraceJitAsmEnabled();

//-------------------------------------------------------------------------------------------------------
// Static members initialization
//-------------------------------------------------------------------------------------------------------
bool JitAsmTracer::s_enabled = false;
const char* JitAsmTracer::s_outputFile = nullptr;
int JitAsmTracer::s_verbosity = 1;
FILE* JitAsmTracer::s_outputStream = nullptr;
size_t JitAsmTracer::s_functionsTraced = 0;
size_t JitAsmTracer::s_totalInstructions = 0;
size_t JitAsmTracer::s_totalCodeBytes = 0;
// Simple mutex replacement for cross-platform compatibility
static bool s_tracingMutex = false;

//-------------------------------------------------------------------------------------------------------
// Constructor/Destructor
//-------------------------------------------------------------------------------------------------------
JitAsmTracer::JitAsmTracer()
    : m_instructions(nullptr)
    , m_instructionCapacity(1024)
    , m_basicBlocks(nullptr)
    , m_basicBlockCapacity(128)
    , m_registerCount(0)
#ifdef ENABLE_CAPSTONE_DISASM
    , m_capstoneHandle(0)
    , m_capstoneInitialized(false)
#endif
{
    // Allocate instruction buffer
    m_instructions = (InstructionInfo*)malloc(m_instructionCapacity * sizeof(InstructionInfo));
    m_basicBlocks = (BasicBlock*)malloc(m_basicBlockCapacity * sizeof(BasicBlock));
    
    // Initialize register stats
    memset(m_registerStats, 0, sizeof(m_registerStats));
    
    // Initialize Capstone if available
#ifdef ENABLE_CAPSTONE_DISASM
    InitializeCapstone();
#endif
}

JitAsmTracer::~JitAsmTracer()
{
    // Cleanup resources
    if (m_instructions)
    {
        free(m_instructions);
    }
    
    if (m_basicBlocks)
    {
        free(m_basicBlocks);
    }
    
#ifdef ENABLE_CAPSTONE_DISASM
    ShutdownCapstone();
#endif
}

//-------------------------------------------------------------------------------------------------------
// Main tracing interface
//-------------------------------------------------------------------------------------------------------
void JitAsmTracer::TraceFunction(Func* func, void* codeAddress, size_t codeSize)
{
    if (!IsEnabled() || !func || !codeAddress || codeSize == 0)
    {
        return;
    }
    
    // Simple mutex lock (not thread-safe, but sufficient for single-threaded tracing)
    if (s_tracingMutex) return; // Already tracing, avoid recursion
    s_tracingMutex = true;
    
    // Get function information
    const char* functionName = GetFunctionName(func);
    const char* sourceInfo = GetSourceInfo(func);
    
    // Print function header
    PrintFunctionHeader(functionName, codeAddress, codeSize);
    
    // Disassemble and analyze the code
#ifdef ENABLE_CAPSTONE_DISASM
    if (m_capstoneInitialized)
    {
        DisassembleCode(codeAddress, codeSize, functionName);
    }
    else
#endif
    {
        // Fallback: basic hex dump
        WriteOutput("Capstone disassembler not available - showing hex dump:\n");
        WriteOutput("Code at %p (%u bytes):\n", codeAddress, (unsigned int)codeSize);
        
        const uint8_t* bytes = static_cast<const uint8_t*>(codeAddress);
        for (size_t i = 0; i < codeSize; i += 16)
        {
            WriteOutput("%p: ", static_cast<const uint8_t*>(codeAddress) + i);
            
            // Hex dump
            for (size_t j = 0; j < 16 && i + j < codeSize; j++)
            {
                WriteOutput("%02x ", bytes[i + j]);
            }
            
            WriteOutput("\n");
        }
    }
    
    PrintFunctionFooter();
    FlushOutput();
    
    // Update statistics
    s_functionsTraced++;
    s_totalCodeBytes += codeSize;
    
    // Release mutex
    s_tracingMutex = false;
}

//-------------------------------------------------------------------------------------------------------
// Capstone integration
//-------------------------------------------------------------------------------------------------------
bool JitAsmTracer::InitializeCapstone()
{
#ifdef ENABLE_CAPSTONE_DISASM
    // Determine architecture and mode based on build target
    cs_arch arch;
    cs_mode mode;
    
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    // x86-64 architecture
    arch = CS_ARCH_X86;
    mode = CS_MODE_64;
#elif defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__)
    // ARM64 architecture (including Apple Silicon)
    arch = CS_ARCH_ARM64;
    mode = CS_MODE_ARM;
#elif defined(_M_IX86) || defined(__i386__)
    // x86 32-bit architecture
    arch = CS_ARCH_X86;
    mode = CS_MODE_32;
#elif defined(_M_ARM) || defined(__arm__)
    // ARM 32-bit architecture
    arch = CS_ARCH_ARM;
    mode = CS_MODE_ARM;
#else
    #error "Unknown architecture for Capstone disassembler"
#endif
    
    if (cs_open(arch, mode, &m_capstoneHandle) == CS_ERR_OK)
    {
        // Enable detailed instruction information
        cs_option(m_capstoneHandle, CS_OPT_DETAIL, CS_OPT_ON);
        
        // Set syntax based on architecture
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_IX86) || defined(__i386__)
        // Intel syntax for x86/x64
        cs_option(m_capstoneHandle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
#endif
        
        m_capstoneInitialized = true;
        return true;
    }
#endif
    
    return false;
}

void JitAsmTracer::ShutdownCapstone()
{
#ifdef ENABLE_CAPSTONE_DISASM
    if (m_capstoneInitialized)
    {
        cs_close(&m_capstoneHandle);
        m_capstoneInitialized = false;
    }
#endif
}

bool JitAsmTracer::DisassembleCode(void* codeAddress, size_t codeSize, const char* functionName)
{
#ifdef ENABLE_CAPSTONE_DISASM
    if (!m_capstoneInitialized)
    {
        return false;
    }
    
    cs_insn* instructions;
    size_t count = cs_disasm(m_capstoneHandle, static_cast<const uint8_t*>(codeAddress), 
                           codeSize, reinterpret_cast<uint64_t>(codeAddress), 0, &instructions);
    
    if (count == 0)
    {
        WriteOutput("Failed to disassemble function %s\n", functionName);
        return false;
    }
    
    WriteOutput("\nDisassembly (%u instructions):\n", (unsigned int)count);
    WriteOutput("Address          | Bytes             | Assembly\n");
    WriteOutput("-----------------|-------------------|------------------\n");
    
    // Convert to our format and analyze
    size_t instructionCount = (count < m_instructionCapacity) ? count : m_instructionCapacity;
    for (size_t i = 0; i < instructionCount; i++)
    {
        InstructionInfo& info = m_instructions[i];
        AnalyzeInstruction(&instructions[i], info);
        PrintInstruction(info, i);
    }
    
    // Perform high-level analysis
    if (s_verbosity >= 2)
    {
        AnalyzeControlFlow(m_instructions, instructionCount);
        AnalyzeRegisterUsage(m_instructions, instructionCount);
        // DetectHotPaths(m_instructions, instructionCount); // TODO: Implement
        
        PrintControlFlowSummary(m_instructions, instructionCount);
        PrintRegisterUsageSummary(m_instructions, instructionCount);
        PrintPerformanceMetrics(m_instructions, instructionCount);
    }
    
    s_totalInstructions += count;
    
    // Free Capstone memory
    cs_free(instructions, count);
    return true;
#else
    return false;
#endif
}

//-------------------------------------------------------------------------------------------------------
// Instruction analysis
//-------------------------------------------------------------------------------------------------------
void JitAsmTracer::AnalyzeInstruction(const cs_insn* insn, InstructionInfo& info)
{
#ifdef ENABLE_CAPSTONE_DISASM
    // Basic information
    info.address = insn->address;
    info.size = insn->size;
    strcpy_s(info.mnemonic, sizeof(info.mnemonic), insn->mnemonic);
    strcpy_s(info.operands, sizeof(info.operands), insn->op_str);
    
    // Control flow analysis
    info.isBranch = false;
    info.isCall = false;
    info.isRet = false;
    info.isJump = false;
    info.target = 0;
    
    // Analyze instruction type
    cs_detail* detail = insn->detail;
    if (detail)
    {
        // Check instruction groups for control flow
        for (int i = 0; i < detail->groups_count; i++)
        {
            switch (detail->groups[i])
            {
            case CS_GRP_BRANCH_RELATIVE:
            case CS_GRP_JUMP:
                info.isBranch = true;
                info.isJump = true;
                break;
            case CS_GRP_CALL:
                info.isCall = true;
                break;
            case CS_GRP_RET:
                info.isRet = true;
                break;
            }
        }
        
        // Extract target address for branches/calls
        if ((info.isBranch || info.isCall) && detail->x86.op_count > 0)
        {
            cs_x86_op* op = &detail->x86.operands[0];
            if (op->type == X86_OP_IMM)
            {
                info.target = op->imm;
            }
        }
        
        // Register analysis
        info.regReadCount = (detail->regs_read_count < 16) ? detail->regs_read_count : 16;
        info.regWriteCount = (detail->regs_write_count < 16) ? detail->regs_write_count : 16;
        
        for (int i = 0; i < info.regReadCount; i++)
        {
            info.registersRead[i] = detail->regs_read[i];
        }
        
        for (int i = 0; i < info.regWriteCount; i++)
        {
            info.registersWritten[i] = detail->regs_write[i];
        }
    }
    else
    {
        // Fallback analysis without detail
        const char* mnemonic = insn->mnemonic;
        
        if (strstr(mnemonic, "call"))
        {
            info.isCall = true;
        }
        else if (strstr(mnemonic, "ret"))
        {
            info.isRet = true;
        }
        else if (strstr(mnemonic, "jmp") || strstr(mnemonic, "j"))
        {
            info.isBranch = true;
            info.isJump = true;
        }
        
        info.regReadCount = 0;
        info.regWriteCount = 0;
    }
#endif
}

void JitAsmTracer::AnalyzeControlFlow(const InstructionInfo* instructions, size_t count)
{
    size_t branchCount = 0;
    size_t callCount = 0;
    size_t retCount = 0;
    
    for (size_t i = 0; i < count; i++)
    {
        const InstructionInfo& info = instructions[i];
        
        if (info.isBranch) branchCount++;
        if (info.isCall) callCount++;
        if (info.isRet) retCount++;
    }
    
    WriteOutput("\nControl Flow Analysis:\n");
    WriteOutput("  Branches: %u\n", (unsigned int)branchCount);
    WriteOutput("  Calls: %u\n", (unsigned int)callCount);
    WriteOutput("  Returns: %u\n", (unsigned int)retCount);
    WriteOutput("  Branch density: %.2f%%\n", (double)branchCount / count * 100.0);
}

void JitAsmTracer::AnalyzeRegisterUsage(const InstructionInfo* instructions, size_t count)
{
    // Reset register stats
    memset(m_registerStats, 0, sizeof(m_registerStats));
    m_registerCount = 0;
    
    // Count register usage
    for (size_t i = 0; i < count; i++)
    {
        const InstructionInfo& info = instructions[i];
        
        // Count reads
        for (int j = 0; j < info.regReadCount; j++)
        {
            int reg = info.registersRead[j];
            RegisterStats* stats = nullptr;
            
            // Find or create register stats
            for (size_t k = 0; k < m_registerCount; k++)
            {
                if (m_registerStats[k].reg == reg)
                {
                    stats = &m_registerStats[k];
                    break;
                }
            }
            
            if (!stats && m_registerCount < 32)
            {
                stats = &m_registerStats[m_registerCount++];
                stats->reg = reg;
                stats->readCount = 0;
                stats->writeCount = 0;
            }
            
            if (stats)
            {
                stats->readCount++;
            }
        }
        
        // Count writes
        for (int j = 0; j < info.regWriteCount; j++)
        {
            int reg = info.registersWritten[j];
            RegisterStats* stats = nullptr;
            
            for (size_t k = 0; k < m_registerCount; k++)
            {
                if (m_registerStats[k].reg == reg)
                {
                    stats = &m_registerStats[k];
                    break;
                }
            }
            
            if (!stats && m_registerCount < 32)
            {
                stats = &m_registerStats[m_registerCount++];
                stats->reg = reg;
                stats->readCount = 0;
                stats->writeCount = 0;
            }
            
            if (stats)
            {
                stats->writeCount++;
            }
        }
    }
    
    // Mark heavy-use registers
    for (size_t i = 0; i < m_registerCount; i++)
    {
        RegisterStats& stats = m_registerStats[i];
        size_t totalUse = stats.readCount + stats.writeCount;
        stats.isHeavyUse = totalUse > count / 10; // More than 10% of instructions
    }
}

void JitAsmTracer::DetectHotPaths(const InstructionInfo* instructions, size_t count)
{
    // Placeholder implementation for hot path detection
    // This would analyze loop patterns and frequently executed code paths
    (void)instructions; // Avoid unused parameter warning
    (void)count;
}

//-------------------------------------------------------------------------------------------------------
// Output formatting
//-------------------------------------------------------------------------------------------------------
void JitAsmTracer::PrintFunctionHeader(const char* functionName, void* codeAddress, size_t codeSize)
{
    WriteOutput("\n");
    WriteOutput("=== JIT COMPILED FUNCTION TRACE ===\n");
    WriteOutput("Function: %s\n", functionName ? functionName : "<unknown>");
    WriteOutput("Address:  %p\n", codeAddress);
    WriteOutput("Size:     %u bytes\n", (unsigned int)codeSize);
    WriteOutput("=====================================\n");
}

void JitAsmTracer::PrintInstruction(const InstructionInfo& info, size_t index)
{
    // Format address
    char addrStr[32];
    sprintf_s(addrStr, sizeof(addrStr), "%p", (void*)info.address);
    
    // Format bytes (show up to 8 bytes)
    char bytesStr[32] = {0};
    // Note: We don't have access to raw bytes in this simplified version
    sprintf_s(bytesStr, sizeof(bytesStr), "<%u bytes>", (unsigned int)info.size);
    
    // Format instruction with analysis markers
    char markers[16] = {0};
    if (info.isCall) strcat_s(markers, sizeof(markers), "C");
    if (info.isBranch) strcat_s(markers, sizeof(markers), "B");
    if (info.isRet) strcat_s(markers, sizeof(markers), "R");
    
    WriteOutput("%16s | %-17s | %-5s %s %s\n", 
                addrStr, bytesStr, markers, info.mnemonic, info.operands);
    
    // Show target address for branches/calls
    if ((info.isBranch || info.isCall) && info.target != 0)
    {
        WriteOutput("                 |                   |       -> %p\n", (void*)info.target);
    }
}

void JitAsmTracer::PrintControlFlowSummary(const InstructionInfo* instructions, size_t count)
{
    WriteOutput("\nControl Flow Summary:\n");
    WriteOutput("====================\n");
    
    size_t branchCount = 0;
    size_t callCount = 0;
    
    for (size_t i = 0; i < count; i++)
    {
        if (instructions[i].isBranch) branchCount++;
        if (instructions[i].isCall) callCount++;
    }
    
    WriteOutput("• Conditional branches: %u\n", (unsigned int)branchCount);
    WriteOutput("• Function calls: %u\n", (unsigned int)callCount);
    WriteOutput("• Branch density: %.1f%%\n", (double)branchCount / count * 100.0);
    
    if (branchCount > count / 4)
    {
        WriteOutput("• ⚠️  High branch density detected - potential performance impact\n");
    }
}

void JitAsmTracer::PrintRegisterUsageSummary(const InstructionInfo* instructions, size_t count)
{
    WriteOutput("\nRegister Usage Summary:\n");
    WriteOutput("=======================\n");
    
    if (m_registerCount == 0)
    {
        WriteOutput("• No detailed register information available\n");
        return;
    }
    
    WriteOutput("• Registers used: %u\n", (unsigned int)m_registerCount);
    WriteOutput("• Heavy-use registers:\n");
    
    for (size_t i = 0; i < m_registerCount; i++)
    {
        const RegisterStats& stats = m_registerStats[i];
        if (stats.isHeavyUse)
        {
            WriteOutput("  - %s: %u reads, %u writes\n", 
                       GetRegisterName(stats.reg), (unsigned int)stats.readCount, (unsigned int)stats.writeCount);
        }
    }
}

void JitAsmTracer::PrintPerformanceMetrics(const InstructionInfo* instructions, size_t count)
{
    WriteOutput("\nPerformance Metrics:\n");
    WriteOutput("====================\n");
    
    size_t mathInstructions = 0;
    size_t memoryInstructions = 0;
    size_t controlInstructions = 0;
    
    for (size_t i = 0; i < count; i++)
    {
        const InstructionInfo& info = instructions[i];
        
        if (IsMathInstruction(info.mnemonic))
        {
            mathInstructions++;
        }
        else if (IsMemoryInstruction(info.mnemonic))
        {
            memoryInstructions++;
        }
        else if (IsControlFlowInstruction(info.mnemonic))
        {
            controlInstructions++;
        }
    }
    
    WriteOutput("• Math operations: %u (%.1f%%)\n", 
               (unsigned int)mathInstructions, (double)mathInstructions / count * 100.0);
    WriteOutput("• Memory operations: %u (%.1f%%)\n", 
               (unsigned int)memoryInstructions, (double)memoryInstructions / count * 100.0);
    WriteOutput("• Control flow: %u (%.1f%%)\n", 
               (unsigned int)controlInstructions, (double)controlInstructions / count * 100.0);
}

void JitAsmTracer::PrintFunctionFooter()
{
    WriteOutput("\n");
    WriteOutput("Legend: [C] Call  [B] Branch  [R] Return\n");
    WriteOutput("────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n");
}

//-------------------------------------------------------------------------------------------------------
// Configuration and utility functions
//-------------------------------------------------------------------------------------------------------
bool JitAsmTracer::IsEnabled()
{
    return s_enabled;
}

void JitAsmTracer::SetEnabled(bool enabled)
{
    s_enabled = enabled;
}

void JitAsmTracer::SetVerbosity(int level)
{
    s_verbosity = level;
}

const char* JitAsmTracer::GetFunctionName(Func* func)
{
    if (!func)
    {
        return "<unknown>";
    }
    
    // Try to get function name from function body
    const JITTimeFunctionBody* functionBody = func->GetJITFunctionBody();
    if (functionBody)
    {
        const char16* name = functionBody->GetDisplayName();
        if (name && *name)
        {
            // Convert char16 to char (simplified)
            static char nameBuffer[256];
            for (int i = 0; i < 255 && name[i] != 0; i++)
            {
                nameBuffer[i] = (char)name[i];
                nameBuffer[i + 1] = 0;
            }
            return nameBuffer;
        }
    }
    
    // Fallback to function number
    static char nameBuffer[64];
    sprintf_s(nameBuffer, sizeof(nameBuffer), "Function_%u", func->GetLocalFunctionId());
    return nameBuffer;
}

const char* JitAsmTracer::GetSourceInfo(Func* func)
{
    if (!func)
    {
        return nullptr;
    }
    
    // Get source context information
    static char sourceBuffer[256];
    sprintf_s(sourceBuffer, sizeof(sourceBuffer), "Line: <unknown>");
    return sourceBuffer;
}

const char* JitAsmTracer::GetRegisterName(int reg)
{
    // Simplified register mapping
    static char regBuffer[16];
    snprintf(regBuffer, sizeof(regBuffer), "r%d", reg);
    return regBuffer;
}

void JitAsmTracer::WriteOutput(const char* format, ...)
{
    if (!s_outputStream)
    {
        s_outputStream = stderr; // Always use stderr for now
    }
    
    if (s_outputStream)
    {
        va_list args;
        va_start(args, format);
        vfprintf(s_outputStream, format, args);
        va_end(args);
    }
}

void JitAsmTracer::FlushOutput()
{
    if (s_outputStream)
    {
        fflush(s_outputStream);
    }
}

bool JitAsmTracer::IsMathInstruction(const char* mnemonic)
{
    return strstr(mnemonic, "add") || strstr(mnemonic, "sub") || 
           strstr(mnemonic, "mul") || strstr(mnemonic, "div") ||
           strstr(mnemonic, "imul") || strstr(mnemonic, "idiv");
}

bool JitAsmTracer::IsMemoryInstruction(const char* mnemonic)
{
    return strstr(mnemonic, "mov") || strstr(mnemonic, "lea") ||
           strstr(mnemonic, "push") || strstr(mnemonic, "pop");
}

bool JitAsmTracer::IsControlFlowInstruction(const char* mnemonic)
{
    return strstr(mnemonic, "jmp") || strstr(mnemonic, "call") ||
           strstr(mnemonic, "ret") || mnemonic[0] == 'j';
}

//-------------------------------------------------------------------------------------------------------
// RAII Trace Scope
//-------------------------------------------------------------------------------------------------------
JitAsmTraceScope::JitAsmTraceScope(Func* func, void* codeAddress, size_t codeSize)
    : m_active(JitAsmTracer::IsEnabled())
{
    if (m_active)
    {
        m_tracer.TraceFunction(func, codeAddress, codeSize);
    }
}

JitAsmTraceScope::~JitAsmTraceScope()
{
    // Automatic cleanup
}

//-------------------------------------------------------------------------------------------------------
// Integration functions (implementations are in JitAsmTraceIntegration.cpp)
//-------------------------------------------------------------------------------------------------------