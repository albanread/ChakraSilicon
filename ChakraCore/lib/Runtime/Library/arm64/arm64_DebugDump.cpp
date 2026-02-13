// Debug dump for JIT-to-C++ calls on Apple ARM64
// Called from arm64_DebugTrampoline in arm64_CallFunction.S
// Enable at runtime: CHAKRA_DUMP_JIT_CALLS=1
//
// Uses an in-memory ring buffer to avoid I/O overhead during hot JIT paths.
// Dumps lazily via atexit() so JIT compilation timing is not disturbed.

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

static int g_dumpEnabled = -1; // -1 = not checked yet

// ---- Ring buffer for deferred output ----
// Each entry captures a snapshot of the call: regs x0-x7, target, callerSP, and 16 stack slots.

struct DumpEntry {
    uintptr_t regs[8];          // x0-x7
    uintptr_t target;           // x16
    uintptr_t callerSP;
    uintptr_t stack[16];        // 16 slots from caller SP
    unsigned  count;            // callInfo.Count
    unsigned  flags;            // callInfo.Flags
    int       seqNo;            // sequential call number
};

static constexpr int RING_SIZE = 4096;  // power-of-2, ~400KB
static DumpEntry g_ring[RING_SIZE];
static int g_ringPos = 0;               // next write slot
static int g_totalCalls = 0;
static bool g_atexitRegistered = false;

static void dump_ring_at_exit()
{
    if (g_totalCalls == 0) return;

    int printed = 0;
    int start, count;
    if (g_totalCalls <= RING_SIZE) {
        start = 0;
        count = g_totalCalls;
    } else {
        start = g_ringPos;  // oldest entry in ring
        count = RING_SIZE;
    }

    fprintf(stderr, "\n=== DEFERRED JIT CALL DUMP (%d calls captured, %d total) ===\n",
            count, g_totalCalls);

    for (int i = 0; i < count; i++) {
        const DumpEntry& e = g_ring[(start + i) % RING_SIZE];

        fprintf(stderr, "\n--- call #%d  target=%p ---\n", e.seqNo, (void*)e.target);
        fprintf(stderr, "  x0 (function)  = %p\n", (void*)e.regs[0]);
        fprintf(stderr, "  x1 (callInfo)  = 0x%lx  [Count=%u Flags=0x%x]\n",
                (unsigned long)((uintptr_t)e.regs[1]), e.count, e.flags);
        for (int r = 2; r <= 7; r++) {
            fprintf(stderr, "  x%d             = %p\n", r, (void*)e.regs[r]);
        }
        fprintf(stderr, "  Caller SP = %p  Stack:\n", (void*)e.callerSP);
        for (int s = 0; s < 16; s++) {
            fprintf(stderr, "    [SP+%3d] = 0x%016lx\n", s * 8, (unsigned long)e.stack[s]);
        }
        printed++;
    }
    fprintf(stderr, "=== END DEFERRED DUMP (%d entries) ===\n", printed);
}

extern "C" void debug_dump_jit_call(uintptr_t* save_area)
{
    // Lazy check env var (once)
    if (g_dumpEnabled == -1)
    {
        const char* env = getenv("CHAKRA_DUMP_JIT_CALLS");
        g_dumpEnabled = (env && env[0] == '1') ? 1 : 0;
    }
    if (!g_dumpEnabled) return;

    // Register atexit handler once
    if (!g_atexitRegistered) {
        atexit(dump_ring_at_exit);
        g_atexitRegistered = true;
    }

    // save_area layout (from arm64_DebugTrampoline):
    //   [0..7] = x0-x7,  [8] = x16 (target),  [9] = caller's SP
    DumpEntry& e = g_ring[g_ringPos];
    for (int i = 0; i < 8; i++) e.regs[i] = save_area[i];
    e.target   = save_area[8];
    e.callerSP = save_area[9];
    e.count    = (unsigned)(save_area[1] & 0xFFFFFF);
    e.flags    = (unsigned)((save_area[1] >> 24) & 0xFF);
    e.seqNo    = g_totalCalls;

    // Snapshot 16 stack slots from caller SP (safe: within our frame)
    uintptr_t* stack = (uintptr_t*)e.callerSP;
    for (int i = 0; i < 16; i++) e.stack[i] = stack[i];

    g_totalCalls++;
    g_ringPos = (g_ringPos + 1) % RING_SIZE;
}
