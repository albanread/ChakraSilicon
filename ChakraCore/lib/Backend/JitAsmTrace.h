//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Backend.h"

#include <capstone/capstone.h>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <vector>

namespace Js
{
    class FunctionBody;
}

//-------------------------------------------------------------------------------------------------------
// JitTraceRequest — self-contained snapshot of a JIT compilation for async tracing
//-------------------------------------------------------------------------------------------------------
struct JitTraceRequest
{
    std::vector<uint8_t> codeBuffer;   // Copy of the generated machine code
    uint64_t             codeAddress;  // Original address (for display)
    size_t               codeSize;     // Size in bytes
    std::string          functionName; // Snapshot of function name
    bool                 isFullJit;    // true = FullJit, false = SimpleJit
};

//-------------------------------------------------------------------------------------------------------
// JitAsmTracer — Capstone-based disassembler (stateless per-request)
//-------------------------------------------------------------------------------------------------------
class JitAsmTracer
{
public:
    JitAsmTracer();
    ~JitAsmTracer();

    // Disassemble a trace request (called on the worker thread)
    void ProcessRequest(const JitTraceRequest& req);

    // Configuration (thread-safe static accessors)
    static bool IsEnabled();
    static void SetEnabled(bool enabled);
    static void SetFullJitOnly(bool fullJitOnly);
    static bool GetFullJitOnly();
    static void SetVerbosity(int level);

private:
    bool InitializeCapstone();
    void ShutdownCapstone();
    bool DisassembleCode(const uint8_t* code, size_t codeSize, uint64_t baseAddr, const char* functionName);

    struct InstructionInfo
    {
        uint64_t address;
        size_t   size;
        char     mnemonic[32];
        char     operands[160];
        bool     isBranch;
        bool     isCall;
        bool     isRet;
        bool     isJump;
        uint64_t target;
        int      registersRead[16];
        int      registersWritten[16];
        int      regReadCount;
        int      regWriteCount;
    };

    void AnalyzeInstruction(const cs_insn* insn, InstructionInfo& info);
    void AnalyzeControlFlow(const InstructionInfo* instructions, size_t count);
    void AnalyzeRegisterUsage(const InstructionInfo* instructions, size_t count);

    void PrintFunctionHeader(const char* functionName, uint64_t codeAddress, size_t codeSize, const char* jitTier);
    void PrintInstruction(const InstructionInfo& info, size_t index);
    void PrintControlFlowSummary(const InstructionInfo* instructions, size_t count);
    void PrintRegisterUsageSummary(const InstructionInfo* instructions, size_t count);
    void PrintPerformanceMetrics(const InstructionInfo* instructions, size_t count);
    void PrintFunctionFooter();

    void WriteOutput(const char* format, ...);
    void FlushOutput();

    static const char* GetMnemonicCategory(const char* mnemonic);
    static bool IsMathInstruction(const char* mnemonic);
    static bool IsMemoryInstruction(const char* mnemonic);
    static bool IsControlFlowInstruction(const char* mnemonic);

    bool IsVolatileRegister(int reg);
    bool IsParameterRegister(int reg);
    bool IsReturnRegister(int reg);
    int  GetInstructionCost(const InstructionInfo& info);

    const char* GetRegisterName(int reg);

    struct BasicBlock
    {
        uint64_t startAddress;
        uint64_t endAddress;
        size_t   instructionCount;
        bool     isLoopHeader;
        bool     isHotPath;
    };

    void IdentifyBasicBlocks(const InstructionInfo* instructions, size_t count,
                             BasicBlock* blocks, size_t& blockCount);

private:
    csh  m_capstoneHandle;
    bool m_capstoneInitialized;

    InstructionInfo* m_instructions;
    size_t           m_instructionCapacity;
    BasicBlock*      m_basicBlocks;
    size_t           m_basicBlockCapacity;

    struct RegisterStats { int reg; size_t readCount; size_t writeCount; bool isHeavyUse; };
    RegisterStats m_registerStats[32];
    size_t        m_registerCount;

    // Statics shared across all instances
    static bool        s_enabled;
    static bool        s_fullJitOnly;
    static int         s_verbosity;
    static FILE*       s_outputStream;
    static size_t      s_functionsTraced;
    static size_t      s_totalInstructions;
    static size_t      s_totalCodeBytes;
};

//-------------------------------------------------------------------------------------------------------
// JitTraceQueue — thread-safe queue with a dedicated worker thread
//
// The JIT compiler thread (or main thread) calls Enqueue() which copies the
// generated code buffer and metadata into a JitTraceRequest.  A background
// pthread picks up requests and runs Capstone disassembly.
//
// Shutdown() drains the queue and joins the worker, guaranteeing that all
// queued traces are flushed to stderr even if the process is about to exit.
//-------------------------------------------------------------------------------------------------------
class JitTraceQueue
{
public:
    // Enqueue a new trace request.  Copies the code buffer from codeAddress.
    // funcName and isFullJit are snapshotted by the caller.
    static void Enqueue(const char* funcName, bool isFullJit,
                        const void* codeAddress, size_t codeSize);

    // Drain queue and join the worker thread.  Safe to call multiple times.
    static void Shutdown();

    // Flush remaining queue items synchronously (called from signal handler).
    // Not async-signal-safe in theory, but sufficient for debugging tools.
    static void FlushSynchronously();

    // Install a SIGSEGV/SIGBUS handler that flushes the queue before dying.
    static void InstallCrashHandler();

    // Start the worker thread (called once during init).
    static void StartWorker();

private:
    static void* WorkerEntry(void* arg);
    static void  ProcessQueue();

    // Queue storage
    static pthread_mutex_t s_mutex;
    static pthread_cond_t  s_cond;
    static pthread_t       s_workerThread;
    static bool            s_workerRunning;
    static bool            s_shutdownRequested;
    static std::vector<JitTraceRequest> s_queue;
};

//-------------------------------------------------------------------------------------------------------
// Macros for easy integration — snapshot metadata and enqueue
//-------------------------------------------------------------------------------------------------------

#define TRACE_JIT_FUNCTION(func, codeAddr, codeSize) \
    do { \
        if (JitAsmTracer::IsEnabled()) { \
            bool _isFullJit = (func) && !(func)->IsSimpleJit(); \
            if (!JitAsmTracer::GetFullJitOnly() || _isFullJit) { \
                const char* _name = nullptr; \
                if (func) { \
                    const JITTimeFunctionBody* _fb = (func)->GetJITFunctionBody(); \
                    if (_fb) { \
                        char _nbuf[256]; \
                        const char16* _wn = _fb->GetDisplayName(); \
                        if (_wn && *_wn) { \
                            int _i; \
                            for (_i = 0; _i < 255 && _wn[_i]; _i++) \
                                _nbuf[_i] = (char)_wn[_i]; \
                            _nbuf[_i] = 0; \
                            _name = _nbuf; \
                        } \
                    } \
                    if (!_name) { \
                        char _nbuf2[64]; \
                        snprintf(_nbuf2, sizeof(_nbuf2), "Function_%u", \
                                 (func)->GetLocalFunctionId()); \
                        _name = _nbuf2; \
                    } \
                } else { \
                    _name = "<unknown>"; \
                } \
                JitTraceQueue::Enqueue(_name, _isFullJit, codeAddr, codeSize); \
            } \
        } \
    } while (0)

#define TRACE_JIT_FUNCTION_IF(condition, func, codeAddr, codeSize) \
    do { \
        if ((condition)) { \
            TRACE_JIT_FUNCTION(func, codeAddr, codeSize); \
        } \
    } while (0)

//-------------------------------------------------------------------------------------------------------
// Configuration flags integration
//-------------------------------------------------------------------------------------------------------
bool IsJitAsmTraceRequested();
void InitializeJitAsmTracing();
void ShutdownJitAsmTracing();