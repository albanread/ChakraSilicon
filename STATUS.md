# ChakraSilicon Project Status

**Last Updated:** 12 February 2026  
**Project:** Port of ChakraCore JavaScript engine to Apple Silicon (ARM64 macOS)

---

## Current Status: JIT WORKING — FIXING REMAINING TEST FAILURES

The interpreter and JIT compiler (SimpleJit + FullJit) run natively on Apple Silicon.
Exception handling works. The core language test suite passes at ~86% (excluding unported features).

## What Works ✅

### Build System
- ✅ All build targets compile successfully
  - `chintx64` - x64 interpreter-only
  - `chjitx64` - x64 JIT (working under Rosetta)
  - `chinta64` - ARM64 interpreter-only
  - `chjita64` - ARM64 JIT (Native Apple Silicon)

### Interpreter
- ✅ Fully working on ARM64 macOS
- ✅ All Basics tests pass in interpreter-only mode (`-NonNative`)

### JIT Compiler
- ✅ SimpleJit — working, all tests pass with `-off:fulljit`
- ✅ FullJit — working, Basics 47/51 pass
- ✅ JIT-compiled code correctly calls variadic C++ entry points (DarwinPCS fix)
- ✅ JIT-compiled code correctly calls other JIT-compiled functions
- ✅ W^X enforcement handled (`MAP_JIT` + `pthread_jit_write_protect_np`)
- ✅ JIT→INT calls work perfectly

### Exception Handling
- ✅ Interpreter exceptions — all pass
- ✅ SimpleJit exceptions — all pass
- ✅ FullJit exceptions — all pass (verified with 10,000 iteration stress test)
- ✅ try/catch/finally — working across JIT and interpreter boundaries
- ✅ Implementation: setjmp/longjmp (bypasses Apple libunwind limitation)

### Repository
- ✅ Converted from submodules to flat structure
- ✅ All ChakraCore source tracked

## Test Results

| Suite | Pass | Fail | Notes |
|-------|------|------|-------|
| Basics | 47 | 4 | Core language |
| TaggedIntegers | 24 | 0 | ✅ All pass |
| switchStatement | 26 | 0 | ✅ All pass |
| TTExecuteBasic | 28 | 0 | ✅ All pass |
| Closures | 25 | 4 | |
| Function | 68 | 13 | |
| Strings | 36 | 7 | |
| strict | 102 | 5 | |
| fieldopts | 108 | 14 | JIT optimizations |
| stackfunc | 79 | 10 | |
| inlining | 44 | 6 | |
| bailout | 29 | 2 | |
| es5 | 41 | 18 | |
| es6 | 128 | 110 | Many need Intl |
| **Total** | **2053** | **849** | **~71% raw, ~86% excluding WASM/Debugger/Intl** |

### Not applicable / Unported (Expected failures)

| Suite | Fails | Reason |
|-------|-------|--------|
| WasmSpec + wasm | 287 | WASM not ported |
| DebuggerCommon | 207 | Debugger protocol not ported |
| Intl | 16 | ICU not linked |

## Completed Fixes

### 1. DarwinPCS Calling Convention
**Problem:** JIT-compiled code put arguments only in registers x0–x7. Apple's variadic ABI requires them on the stack.  
**Fix:** JIT backend emits STR instructions to spill all register args to the outgoing stack area before BLR calls. `DECLARE_ARGS_VARARRAY` uses `va_list` on Apple ARM64. `CALL_ENTRYPOINT` uses non-variadic function pointer cast with stack duplication.  
**Files:** `InterpreterThunkEmitter.cpp`, `Arguments.h`, `LowerMD.cpp`

### 2. W^X Memory Protection
**Problem:** `mprotect(RWX)` causes SIGBUS on Apple Silicon.  
**Fix:** `MAP_JIT` flag on mmap, `pthread_jit_write_protect_np()` to toggle W↔X per-thread.  
**Files:** `CustomHeap.cpp`, `VirtualAllocWrapper.cpp`

### 3. Exception Handling
**Problem:** Apple's libunwind cannot transition from dynamic `.eh_frame` (JIT) to static compact `__unwind_info` (C++ runtime).  
**Fix:** setjmp/longjmp-based exception handling for Apple ARM64.  
**Files:** `JavascriptExceptionOperators.cpp`, `ThreadContext.h`

### 4. DWARF .eh_frame Emission
**Problem:** Wrong DWARF register numbers, missing CFA directives, incorrect FDE encoding.  
**Fix:** Corrected all CFI directives for ARM64 macOS.  
**Files:** `EhFrameCFI.inc`, `EhFrame.cpp`, `PrologEncoder.cpp`, `XDataAllocator.cpp`

### 5. Prolog/Epilog Code Generation
**Problem:** STP/LDP pair instructions didn't match DWARF CFI expectations and caused instability.  
**Fix:** Split into individual STR/LDR with per-register CFI.  
**Files:** `LowerMD.cpp`, `PrologEncoderMD.cpp`

### 6. Debug Output Pollution
**Problem:** fprintf debug messages in JIT pipeline were polluting stdout, causing ~2800 test failures.  
**Fix:** Removed all debug fprintf from Encoder.cpp, PDataManager.cpp, XDataAllocator.cpp, JITOutput.cpp, JitAsmTrace.cpp.

## Known Issues / TODO

- **Remaining test failures:** ~14% of applicable tests fail.
  - Some JIT codegen issues likely remain in optimizer paths (fieldopts, inlining, bailout).
  - ES6 failures — many are Intl-dependent, some may be real bugs.
  - 4 Basics failures — worth investigating individually.
- **Unported Features:**
  - **WASM**: Would need same DarwinPCS and W^X fixes applied to WASM JIT.
  - **Debugger protocol**: VS Code debug adapter, Windows-centric.
  - **Intl**: Needs ICU library linkage.
  - **iOS**: Would need JIT entitlements.

## Build Commands

```bash
# Debug build
cd build/chjita64_debug
ninja -j10

# Run
./bin/ch/ch script.js

# Run with flags
./bin/ch/ch -NoNative script.js        # Interpreter only
./bin/ch/ch -off:fulljit script.js     # SimpleJit only
./bin/ch/ch -off:simpleJit script.js   # FullJit only
./bin/ch/ch -TraceJitAsm script.js     # Show JIT disassembly

# Run Tests
python3 ChakraCore/test/runtests.py \
  -b $(pwd)/bin/ch/ch \
  -d --show-passes --variants interpreted Basics
```
