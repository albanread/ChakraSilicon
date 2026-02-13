//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"
#include "JitAsmTrace.h"

#include <capstone/capstone.h>
#include <sys/ucontext.h>

// External function declaration
extern "C" bool IsTraceJitAsmEnabled();

//-------------------------------------------------------------------------------------------------------
// JitAsmTracer — static members
//-------------------------------------------------------------------------------------------------------
bool        JitAsmTracer::s_enabled          = false;
bool        JitAsmTracer::s_fullJitOnly      = false;
int         JitAsmTracer::s_verbosity        = 1;
FILE*       JitAsmTracer::s_outputStream     = nullptr;
size_t      JitAsmTracer::s_functionsTraced  = 0;
size_t      JitAsmTracer::s_totalInstructions = 0;
size_t      JitAsmTracer::s_totalCodeBytes   = 0;

//-------------------------------------------------------------------------------------------------------
// JitTraceQueue — static members
//-------------------------------------------------------------------------------------------------------
pthread_mutex_t             JitTraceQueue::s_mutex           = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t              JitTraceQueue::s_cond            = PTHREAD_COND_INITIALIZER;
pthread_t                   JitTraceQueue::s_workerThread    = 0;
bool                        JitTraceQueue::s_workerRunning   = false;
bool                        JitTraceQueue::s_shutdownRequested = false;
std::vector<JitTraceRequest> JitTraceQueue::s_queue;

//-------------------------------------------------------------------------------------------------------
// Constructor/Destructor
//-------------------------------------------------------------------------------------------------------
JitAsmTracer::JitAsmTracer()
    : m_instructions(nullptr)
    , m_instructionCapacity(1024)
    , m_basicBlocks(nullptr)
    , m_basicBlockCapacity(128)
    , m_registerCount(0)
    , m_capstoneHandle(0)
    , m_capstoneInitialized(false)
{
    m_instructions = (InstructionInfo*)malloc(m_instructionCapacity * sizeof(InstructionInfo));
    m_basicBlocks  = (BasicBlock*)malloc(m_basicBlockCapacity * sizeof(BasicBlock));
    memset(m_registerStats, 0, sizeof(m_registerStats));
    InitializeCapstone();
}

JitAsmTracer::~JitAsmTracer()
{
    if (m_instructions) free(m_instructions);
    if (m_basicBlocks)  free(m_basicBlocks);
    ShutdownCapstone();
}

//-------------------------------------------------------------------------------------------------------
// ProcessRequest — main entry for the worker thread (no Func* dependency)
//-------------------------------------------------------------------------------------------------------
void JitAsmTracer::ProcessRequest(const JitTraceRequest& req)
{
    const char* jitTier = req.isFullJit ? "FullJit" : "SimpleJit";
    PrintFunctionHeader(req.functionName.c_str(), req.codeAddress, req.codeSize, jitTier);

    if (m_capstoneInitialized && !req.codeBuffer.empty())
    {
        DisassembleCode(req.codeBuffer.data(), req.codeSize, req.codeAddress,
                        req.functionName.c_str());
    }
    else
    {
        WriteOutput("Capstone not available or empty buffer — skipping disassembly\n");
    }

    PrintFunctionFooter();
    FlushOutput();

    s_functionsTraced++;
    s_totalCodeBytes += req.codeSize;
}

//-------------------------------------------------------------------------------------------------------
// Capstone integration
//-------------------------------------------------------------------------------------------------------
bool JitAsmTracer::InitializeCapstone()
{
    cs_arch arch;
    cs_mode mode;

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    arch = CS_ARCH_X86;  mode = CS_MODE_64;
#elif defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__)
    arch = CS_ARCH_AARCH64; mode = CS_MODE_ARM;
#elif defined(_M_IX86) || defined(__i386__)
    arch = CS_ARCH_X86;  mode = CS_MODE_32;
#elif defined(_M_ARM) || defined(__arm__)
    arch = CS_ARCH_ARM;  mode = CS_MODE_ARM;
#else
    #error "Unknown architecture for Capstone"
#endif

    if (cs_open(arch, mode, &m_capstoneHandle) == CS_ERR_OK)
    {
        cs_option(m_capstoneHandle, CS_OPT_DETAIL, CS_OPT_ON);
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_IX86) || defined(__i386__)
        cs_option(m_capstoneHandle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
#endif
        m_capstoneInitialized = true;
        return true;
    }
    return false;
}

void JitAsmTracer::ShutdownCapstone()
{
    if (m_capstoneInitialized) { cs_close(&m_capstoneHandle); m_capstoneInitialized = false; }
}

bool JitAsmTracer::DisassembleCode(const uint8_t* code, size_t codeSize,
                                    uint64_t baseAddr, const char* functionName)
{
    if (!m_capstoneInitialized) return false;

    cs_insn* instructions;
    size_t count = cs_disasm(m_capstoneHandle, code, codeSize, baseAddr, 0, &instructions);
    if (count == 0) { WriteOutput("Failed to disassemble %s\n", functionName); return false; }

    WriteOutput("\nDisassembly (%u instructions):\n", (unsigned)count);
    WriteOutput("Address          | Bytes             | Assembly\n");
    WriteOutput("-----------------|-------------------|------------------\n");

    size_t n = (count < m_instructionCapacity) ? count : m_instructionCapacity;
    for (size_t i = 0; i < n; i++)
    {
        AnalyzeInstruction(&instructions[i], m_instructions[i]);
        PrintInstruction(m_instructions[i], i);
    }

    if (s_verbosity >= 2)
    {
        AnalyzeControlFlow(m_instructions, n);
        AnalyzeRegisterUsage(m_instructions, n);
        PrintControlFlowSummary(m_instructions, n);
        PrintRegisterUsageSummary(m_instructions, n);
        PrintPerformanceMetrics(m_instructions, n);
    }

    s_totalInstructions += count;
    cs_free(instructions, count);
    return true;
}

//-------------------------------------------------------------------------------------------------------
// Instruction analysis
//-------------------------------------------------------------------------------------------------------
void JitAsmTracer::AnalyzeInstruction(const cs_insn* insn, InstructionInfo& info)
{
    info.address = insn->address;
    info.size    = insn->size;
    strncpy(info.mnemonic, insn->mnemonic, sizeof(info.mnemonic) - 1);
    info.mnemonic[sizeof(info.mnemonic) - 1] = 0;
    strncpy(info.operands, insn->op_str, sizeof(info.operands) - 1);
    info.operands[sizeof(info.operands) - 1] = 0;

    info.isBranch = false; info.isCall = false;
    info.isRet    = false; info.isJump = false;
    info.target   = 0;

    cs_detail* detail = insn->detail;
    if (detail)
    {
        for (int i = 0; i < detail->groups_count; i++)
        {
            switch (detail->groups[i])
            {
            case CS_GRP_BRANCH_RELATIVE:
            case CS_GRP_JUMP:  info.isBranch = true; info.isJump = true; break;
            case CS_GRP_CALL:  info.isCall = true; break;
            case CS_GRP_RET:   info.isRet  = true; break;
            }
        }
        info.regReadCount  = (detail->regs_read_count  < 16) ? detail->regs_read_count  : 16;
        info.regWriteCount = (detail->regs_write_count < 16) ? detail->regs_write_count : 16;
        for (int i = 0; i < info.regReadCount;  i++) info.registersRead[i]    = detail->regs_read[i];
        for (int i = 0; i < info.regWriteCount; i++) info.registersWritten[i] = detail->regs_write[i];
    }
    else
    {
        if (strstr(insn->mnemonic, "call")) info.isCall = true;
        else if (strstr(insn->mnemonic, "ret")) info.isRet = true;
        else if (insn->mnemonic[0] == 'b') { info.isBranch = true; info.isJump = true; }
        info.regReadCount = 0; info.regWriteCount = 0;
    }
}

void JitAsmTracer::AnalyzeControlFlow(const InstructionInfo* insts, size_t count)
{
    size_t br = 0, ca = 0, re = 0;
    for (size_t i = 0; i < count; i++) { if (insts[i].isBranch) br++; if (insts[i].isCall) ca++; if (insts[i].isRet) re++; }
    WriteOutput("\nControl Flow Analysis:\n");
    WriteOutput("  Branches: %u  Calls: %u  Returns: %u\n", (unsigned)br, (unsigned)ca, (unsigned)re);
    WriteOutput("  Branch density: %.2f%%\n", count ? (double)br / count * 100.0 : 0.0);
}

void JitAsmTracer::AnalyzeRegisterUsage(const InstructionInfo* insts, size_t count)
{
    memset(m_registerStats, 0, sizeof(m_registerStats));
    m_registerCount = 0;
    for (size_t i = 0; i < count; i++)
    {
        for (int j = 0; j < insts[i].regReadCount; j++)
        {
            int r = insts[i].registersRead[j];
            RegisterStats* s = nullptr;
            for (size_t k = 0; k < m_registerCount; k++) if (m_registerStats[k].reg == r) { s = &m_registerStats[k]; break; }
            if (!s && m_registerCount < 32) { s = &m_registerStats[m_registerCount++]; s->reg = r; s->readCount = 0; s->writeCount = 0; }
            if (s) s->readCount++;
        }
        for (int j = 0; j < insts[i].regWriteCount; j++)
        {
            int r = insts[i].registersWritten[j];
            RegisterStats* s = nullptr;
            for (size_t k = 0; k < m_registerCount; k++) if (m_registerStats[k].reg == r) { s = &m_registerStats[k]; break; }
            if (!s && m_registerCount < 32) { s = &m_registerStats[m_registerCount++]; s->reg = r; s->readCount = 0; s->writeCount = 0; }
            if (s) s->writeCount++;
        }
    }
    for (size_t i = 0; i < m_registerCount; i++)
    {
        m_registerStats[i].isHeavyUse = (m_registerStats[i].readCount + m_registerStats[i].writeCount) > count / 10;
    }
}

//-------------------------------------------------------------------------------------------------------
// Output formatting
//-------------------------------------------------------------------------------------------------------
void JitAsmTracer::PrintFunctionHeader(const char* name, uint64_t addr, size_t size, const char* tier)
{
    WriteOutput("\n=== JIT COMPILED FUNCTION TRACE [%s] ===\n", tier ? tier : "unknown");
    WriteOutput("Function: %s\n", name ? name : "<unknown>");
    WriteOutput("JIT Tier: %s\n", tier);
    WriteOutput("Address:  0x%llx\n", (unsigned long long)addr);
    WriteOutput("Size:     %u bytes\n", (unsigned)size);
    WriteOutput("=====================================\n");
}

void JitAsmTracer::PrintInstruction(const InstructionInfo& info, size_t /*index*/)
{
    char markers[8] = {};
    if (info.isCall)   strcat(markers, "C");
    if (info.isBranch) strcat(markers, "B");
    if (info.isRet)    strcat(markers, "R");
    WriteOutput("%016llx | <%u bytes>%*s | %-5s %s %s\n",
                (unsigned long long)info.address,
                (unsigned)info.size,
                (int)(17 - 3 - (info.size > 9 ? 2 : 1) - 8), "",
                markers, info.mnemonic, info.operands);
}

void JitAsmTracer::PrintControlFlowSummary(const InstructionInfo* insts, size_t count)
{
    size_t br = 0, ca = 0;
    for (size_t i = 0; i < count; i++) { if (insts[i].isBranch) br++; if (insts[i].isCall) ca++; }
    WriteOutput("\nControl Flow Summary:\n");
    WriteOutput("  Branches: %u   Calls: %u   Density: %.1f%%\n",
                (unsigned)br, (unsigned)ca, count ? (double)br / count * 100.0 : 0.0);
}

void JitAsmTracer::PrintRegisterUsageSummary(const InstructionInfo*, size_t)
{
    WriteOutput("\nRegister Usage Summary: %u registers tracked\n", (unsigned)m_registerCount);
    for (size_t i = 0; i < m_registerCount; i++)
    {
        if (m_registerStats[i].isHeavyUse)
            WriteOutput("  %s: %u reads, %u writes\n",
                        GetRegisterName(m_registerStats[i].reg),
                        (unsigned)m_registerStats[i].readCount,
                        (unsigned)m_registerStats[i].writeCount);
    }
}

void JitAsmTracer::PrintPerformanceMetrics(const InstructionInfo* insts, size_t count)
{
    size_t math = 0, mem = 0, ctrl = 0;
    for (size_t i = 0; i < count; i++)
    {
        if (IsMathInstruction(insts[i].mnemonic))         math++;
        else if (IsMemoryInstruction(insts[i].mnemonic))  mem++;
        else if (IsControlFlowInstruction(insts[i].mnemonic)) ctrl++;
    }
    WriteOutput("\nInstruction Mix: math=%u  mem=%u  ctrl=%u  other=%u\n",
                (unsigned)math, (unsigned)mem, (unsigned)ctrl,
                (unsigned)(count - math - mem - ctrl));
}

void JitAsmTracer::PrintFunctionFooter()
{
    WriteOutput("Legend: [C] Call  [B] Branch  [R] Return\n");
    WriteOutput("───────────────────────────────────────────────────────────\n");
}

//-------------------------------------------------------------------------------------------------------
// Configuration accessors
//-------------------------------------------------------------------------------------------------------
bool JitAsmTracer::IsEnabled()              { return s_enabled; }
void JitAsmTracer::SetEnabled(bool v)       { s_enabled = v; }
void JitAsmTracer::SetFullJitOnly(bool v)   { s_fullJitOnly = v; }
bool JitAsmTracer::GetFullJitOnly()         { return s_fullJitOnly; }
void JitAsmTracer::SetVerbosity(int v)      { s_verbosity = v; }

const char* JitAsmTracer::GetRegisterName(int reg)
{
    static char buf[16]; snprintf(buf, sizeof(buf), "r%d", reg); return buf;
}

void JitAsmTracer::WriteOutput(const char* format, ...)
{
    if (!s_outputStream) s_outputStream = stderr;
    va_list args; va_start(args, format);
    vfprintf(s_outputStream, format, args);
    va_end(args);
}

void JitAsmTracer::FlushOutput()
{
    if (s_outputStream) fflush(s_outputStream);
}

bool JitAsmTracer::IsMathInstruction(const char* m)
{ return strstr(m,"add")||strstr(m,"sub")||strstr(m,"mul")||strstr(m,"div"); }
bool JitAsmTracer::IsMemoryInstruction(const char* m)
{ return strstr(m,"ldr")||strstr(m,"str")||strstr(m,"ldp")||strstr(m,"stp")||strstr(m,"mov")||strstr(m,"lea"); }
bool JitAsmTracer::IsControlFlowInstruction(const char* m)
{ return m[0]=='b'||strstr(m,"bl")||strstr(m,"ret")||strstr(m,"cbz")||strstr(m,"cbnz"); }

// Stubs for unused analysis helpers
bool JitAsmTracer::IsVolatileRegister(int)  { return false; }
bool JitAsmTracer::IsParameterRegister(int) { return false; }
bool JitAsmTracer::IsReturnRegister(int)    { return false; }
int  JitAsmTracer::GetInstructionCost(const InstructionInfo&) { return 1; }
const char* JitAsmTracer::GetMnemonicCategory(const char*) { return "other"; }

void JitAsmTracer::IdentifyBasicBlocks(const InstructionInfo*, size_t,
                                        BasicBlock*, size_t&) {}

//-------------------------------------------------------------------------------------------------------
// JitTraceQueue — thread-safe queue with a dedicated worker thread
//-------------------------------------------------------------------------------------------------------
void JitTraceQueue::Enqueue(const char* funcName, bool isFullJit,
                            const void* codeAddress, size_t codeSize)
{
    JitTraceRequest req;
    req.functionName = funcName ? funcName : "<unknown>";
    req.isFullJit    = isFullJit;
    req.codeAddress  = reinterpret_cast<uint64_t>(codeAddress);
    req.codeSize     = codeSize;

    // Copy the machine code buffer so the worker thread owns it
    const uint8_t* src = static_cast<const uint8_t*>(codeAddress);
    req.codeBuffer.assign(src, src + codeSize);

    pthread_mutex_lock(&s_mutex);
    if (!s_workerRunning && !s_shutdownRequested)
    {
        // Lazy-start the worker on first enqueue
        StartWorker();
    }
    s_queue.push_back(std::move(req));
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
}

void JitTraceQueue::StartWorker()
{
    // Must be called with s_mutex held
    s_shutdownRequested = false;
    int rc = pthread_create(&s_workerThread, nullptr, WorkerEntry, nullptr);
    if (rc == 0) s_workerRunning = true;
}

void* JitTraceQueue::WorkerEntry(void* /*arg*/)
{
    // Thread-local JitAsmTracer (owns its own Capstone handle)
    JitAsmTracer tracer;

    pthread_mutex_lock(&s_mutex);
    while (true)
    {
        // Wait for work or shutdown
        while (s_queue.empty() && !s_shutdownRequested)
        {
            pthread_cond_wait(&s_cond, &s_mutex);
        }

        if (s_queue.empty() && s_shutdownRequested)
            break;

        // Grab one request
        JitTraceRequest req = std::move(s_queue.front());
        s_queue.erase(s_queue.begin());
        pthread_mutex_unlock(&s_mutex);

        // Process outside the lock
        tracer.ProcessRequest(req);

        pthread_mutex_lock(&s_mutex);
    }
    pthread_mutex_unlock(&s_mutex);
    return nullptr;
}

void JitTraceQueue::Shutdown()
{
    pthread_mutex_lock(&s_mutex);
    if (!s_workerRunning)
    {
        pthread_mutex_unlock(&s_mutex);
        return;
    }
    s_shutdownRequested = true;
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);

    // Join — blocks until the worker has drained the queue and exited
    pthread_join(s_workerThread, nullptr);
    s_workerRunning = false;
}

//-------------------------------------------------------------------------------------------------------
// FlushSynchronously — called from signal handler when the process is crashing.
// We bypass the mutex (process is dying, only this handler is running the queue)
// and process all remaining items directly in the handler's context.
//-------------------------------------------------------------------------------------------------------
void JitTraceQueue::FlushSynchronously()
{
    if (s_workerRunning)
    {
        // Tell the worker to drain its queue and exit
        s_shutdownRequested = true;
        pthread_cond_signal(&s_cond);

        // Wait for the worker to finish — it will process all remaining items
        pthread_join(s_workerThread, nullptr);
        s_workerRunning = false;
    }

    // Process anything that might still be left (shouldn't be, but safety net)
    if (!s_queue.empty())
    {
        JitAsmTracer tracer;
        for (size_t i = 0; i < s_queue.size(); i++)
        {
            tracer.ProcessRequest(s_queue[i]);
        }
        s_queue.clear();
    }

    fprintf(stderr, "\n=== JIT Trace: crash-flush completed ===\n");
    fflush(stderr);
}

//-------------------------------------------------------------------------------------------------------
// Signal handler for SIGSEGV / SIGBUS — flush remaining traces, then re-raise
//-------------------------------------------------------------------------------------------------------
static void JitTraceCrashHandler(int sig, siginfo_t* info, void* ucontext)
{
    // Avoid re-entrancy
    static volatile int s_inHandler = 0;
    if (s_inHandler) { _Exit(128 + sig); }
    s_inHandler = 1;

    // Extract crash details from signal context
    ucontext_t* uc = static_cast<ucontext_t*>(ucontext);
#if defined(__aarch64__) && defined(__APPLE__)
    uint64_t pc = uc->uc_mcontext->__ss.__pc;
    uint64_t sp = uc->uc_mcontext->__ss.__sp;
    uint64_t lr = uc->uc_mcontext->__ss.__lr;
    uint64_t fp = uc->uc_mcontext->__ss.__fp;
    fprintf(stderr, "\n\n!!! CRASH (signal %d) at PC=0x%llx SP=0x%llx LR=0x%llx FP=0x%llx\n",
            sig, pc, sp, lr, fp);
    fprintf(stderr, "    Fault address: %p\n", info->si_addr);
    // Print registers x0-x28
    fprintf(stderr, "    Registers:\n");
    for (int i = 0; i < 29; i++) {
        fprintf(stderr, "      x%-2d = 0x%016llx", i, uc->uc_mcontext->__ss.__x[i]);
        if (i % 4 == 3) fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
#else
    fprintf(stderr, "\n\n!!! CRASH (signal %d)\n", sig);
    fprintf(stderr, "    Fault address: %p\n", info->si_addr);
#endif
    fprintf(stderr, "    Flushing JIT trace queue...\n");
    fflush(stderr);

    JitTraceQueue::FlushSynchronously();

    // Restore default handler and re-raise so the OS produces a core dump
    signal(sig, SIG_DFL);
    raise(sig);
}

void JitTraceQueue::InstallCrashHandler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = JitTraceCrashHandler;
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction with siginfo_t
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
}
