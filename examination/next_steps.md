# Apple Silicon JIT Investigation - Next Steps

**Status:** JIT Completely Broken - Root Cause Analysis Complete
**Date:** January 2025
**Platform:** macOS ARM64 (Apple Silicon M1/M2/M3)
**Priority:** CRITICAL

---

## Executive Summary

The Apple Silicon JIT port is currently **completely non-functional** for exception handling. While basic code generation works, any exception thrown across function boundaries causes immediate termination. This document consolidates all research findings and provides a clear path forward.

### Current State

| Component | Status | Notes |
|-----------|--------|-------|
| Basic JIT compilation | ✅ Working | Code generates and executes |
| Same-function exceptions | ✅ Working | No unwinding needed |
| Cross-function exceptions | ❌ BROKEN | Process terminates |
| Interpreter exceptions | ❌ BROKEN | Even interpreter-only mode fails |
| DWARF infrastructure | ⚠️ Partial | Code exists but untested |
| Apple Silicon constraints | ✅ Documented | STP/LDP prohibition understood |

### Why This Matters

Without proper exception handling:
- JavaScript `try/catch` across function calls doesn't work
- Built-in functions that throw exceptions cause crashes
- Real-world JavaScript code fails
- The JIT is essentially unusable

---

## Root Cause Analysis

### The Problem

When an exception is thrown in JavaScript code that crosses JIT-compiled function boundaries:

```javascript
function thrower() {
    throw new Error("test");
}

function catcher() {
    try {
        thrower();  // <- Exception thrown here
    } catch (e) {
        print("Caught: " + e.message);  // <- Never reached
    }
}

catcher();  // Result: "libc++abi: terminating due to uncaught exception"
```

**What Should Happen:**
1. JavaScript `throw` → C++ `throw Js::JavascriptException`
2. libunwind begins stack unwinding
3. For each frame, libunwind reads unwind metadata (DWARF or compact unwind)
4. Registers are restored, stack is unwound
5. Landing pad (catch handler) is found and executed

**What Actually Happens:**
1. JavaScript `throw` → C++ `throw Js::JavascriptException`
2. libunwind begins stack unwinding
3. For JIT frame: **No unwind metadata found** (or metadata is incorrect)
4. libunwind cannot continue
5. Process terminates: `libc++abi: terminating due to uncaught exception`

### Critical Discovery: Even Interpreter Fails

```bash
# Test with JIT completely disabled
./ch -maxInterpretCount:1 -off:jitloopbody test_cross_function.js
# Result: STILL CRASHES

# This means the problem is NOT JIT-specific
# It's a fundamental runtime integration issue
```

**Implications:**
- The issue is deeper than missing DWARF metadata
- ChakraCore's C++ exception integration on ARM64 Unix is incomplete
- The problem affects both JIT and interpreter
- This is not just a "generate CFI directives" problem

---

## Apple Silicon Specific Constraints

### 1. Prohibited Instructions: STP/LDP with SP

Apple Silicon **prohibits Store Pair (STP) and Load Pair (LDP)** instructions when using the stack pointer:

```armasm
# PROHIBITED
stp x19, x20, [sp, #offset]     # ❌ Store pair with SP
ldp x19, x20, [sp, #offset]     # ❌ Load pair with SP

# REQUIRED ALTERNATIVE
str x19, [sp, #offset]          # ✅ Individual store
str x20, [sp, #offset+8]        # ✅ Individual store
ldr x19, [sp, #offset]          # ✅ Individual load
ldr x20, [sp, #offset+8]        # ✅ Individual load
```

**Impact on Unwinding:**
- Each individual STR/LDR needs its own CFI directive
- Cannot use pair-save optimization for unwind info
- More complex DWARF generation required

**Exception:**
```armasm
# This SPECIFIC pattern is allowed (canonical frame creation):
stp x29, x30, [sp, #-16]!       # ✅ Pre-indexed frame setup
```

### 2. Memory Allocation with MAP_JIT

All JIT code memory must use `MAP_JIT` flag:

```cpp
void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,  // MAP_JIT REQUIRED
                    -1, 0);
```

### 3. Write XOR Execute (W^X)

Memory is never simultaneously writable and executable:

```cpp
// Enable writing, disable execution
pthread_jit_write_protect_np(false);
// ... write code ...

// Enable execution, disable writing  
pthread_jit_write_protect_np(true);
// ... execute code ...
```

### 4. Stack Alignment

Stack must be 16-byte aligned at function boundaries:

```cpp
// Correct
sub sp, sp, #32      // 16-byte aligned
sub sp, sp, #64      // 16-byte aligned

// Incorrect
sub sp, sp, #24      // Only 8-byte aligned ❌
sub sp, sp, #40      // Only 8-byte aligned ❌
```

---

## What Has Been Implemented

### 1. ARM64 PrologEncoderMD (Phase 3)

**Files:**
- `lib/Backend/arm64/PrologEncoderMD.h`
- `lib/Backend/arm64/PrologEncoderMD.cpp`

**Purpose:** Analyze ARM64 prolog instructions and extract unwind information

**Capabilities:**
- Classifies instructions (STP/STR, SUB sp, ADD fp, etc.)
- Extracts register save information
- Handles both standard ARM64 and Apple Silicon patterns
- Determines stack allocation sizes
- Identifies frame pointer setup

**Status:** ✅ Implemented and compiling

### 2. ARM64 DWARF Register Mapping

**File:** `lib/Backend/EhFrame.cpp`

**Added:**
- Complete ARM64 DWARF register number mapping per ABI spec
- x0-x30 → DWARF 0-30
- SP (x31) → DWARF 31
- d0-d29 → DWARF 64-93
- LR (x30) as return address register

**Status:** ✅ Implemented

### 3. ARM64 CFI Emission

**File:** `lib/Backend/PrologEncoder.cpp`

**Added ARM64 support for:**
- Register pair saves (STP)
- Individual register saves (STR) - for Apple Silicon
- Stack allocation (SUB sp, sp, #imm)
- NEON/FP register saves
- Frame pointer setup (ADD fp, sp, #offset)

**Status:** ✅ Implemented and compiling

### 4. Build System Integration

**Files:** `lib/Backend/CMakeLists.txt`, `lib/Backend/Backend.h`

**Changes:**
- Added ARM64 to PrologEncoder build
- Included PrologEncoder.h for ARM64 target
- Enabled EhFrame.cpp for ARM64

**Status:** ✅ Complete - full project builds

### 5. Diagnostic Infrastructure

**Files:**
- `lib/Backend/EmitBuffer.h` / `.cpp` - Enhanced JIT disassembly with function names
- `lib/Backend/arm64/LowerMD.cpp` - Call site diagnostics
- `lib/Runtime/Language/InterpreterStackFrame.cpp` - Thunk entry logging
- `lib/Backend/JitCallDiagnostics.h` / `.cpp` - Register state capture

**Outputs:**
- `/tmp/chakra_jit_disasm_<pid>.txt` - JIT disassembly
- `/tmp/jit_call_diagnostics.txt` - Register state before/after calls
- `/tmp/interpreter_thunk_dump.txt` - Interpreter entry points

**Status:** ✅ Complete - provides detailed debugging information

---

## What's Still Missing

### Critical Issues

#### 1. DWARF Registration Not Verified

**Problem:** We don't know if `__REGISTER_FRAME` is being called for JIT functions.

**Need to verify:**
```cpp
// In PDataManager.cpp
void PDataManager::RegisterPdata(...) {
    fprintf(stderr, "[EH] Registering frame: start=%p, size=%zu\n",
            functionStart, functionSize);
    __REGISTER_FRAME(pdataStart);
}
```

**Test:** Check if these log messages appear when JIT functions are compiled.

#### 2. DWARF Correctness Not Validated

**Problem:** The generated DWARF CFI may be incorrect or incomplete.

**Need to verify:**
```bash
# Dump generated DWARF
llvm-dwarfdump --eh-frame build/libChakraCore.dylib

# Check for:
# - CIE with correct initial rules
# - FDEs for JIT functions
# - Proper DW_CFA_* directives
# - Correct register offsets
```

**Potential Issues:**
- CFA offset calculations may be wrong
- Register save offsets may not match actual stack layout
- Frame pointer switch may not emit `DW_CFA_def_cfa_register`
- Individual STR (Apple Silicon) may lack proper CFI

#### 3. Interpreter Exception Handling Broken

**Problem:** Even with JIT disabled, cross-function exceptions fail.

**This indicates:**
- Not a JIT-specific issue
- Fundamental C++ exception runtime integration problem
- PAL (Platform Abstraction Layer) may be incomplete for ARM64 Unix
- JavascriptException may not integrate correctly with libunwind

**Need to investigate:**
- Does `throw Js::JavascriptException` properly invoke C++ exception mechanism?
- Are exception filters registered correctly?
- Does the PAL signal handler work on ARM64 macOS?
- Is there x64 Unix code we can compare against?

#### 4. Split Epilog Not Handled

**Problem:** Functions with `try/catch` use a special "split epilog" model.

**ChakraCore's EH Model (from PR #4182):**
- Functions with `HasTry()` save ALL callee-saved registers
- Normal exit: deallocate locals + restore registers
- EH exit (`EnsureEHEpilogLabel`): skip locals deallocation, only restore registers
- Helper `arm64_CallEhFrame` creates synthetic frame

**Our DWARF Implementation:**
- Only covers normal prolog/epilog
- Doesn't describe EH entry point
- May need separate FDE or additional CFI for EH path

---

## Path Forward

### Phase 1: Verify DWARF Generation (1-2 days)

**Goal:** Confirm DWARF is being generated and registered

**Tasks:**

1. **Add Debug Logging**
   ```cpp
   // In PrologEncoder::EncodeInstr()
   #ifdef _DEBUG
   fprintf(stderr, "[DWARF] Op=%d, offset=%zu, cfa=%u\n",
           unwindCodeOp, currentInstrOffset, cfaWordOffset);
   #endif
   ```

2. **Verify __REGISTER_FRAME Calls**
   ```cpp
   // In PDataManager::RegisterPdata()
   fprintf(stderr, "[EH] Registering: %p-%p, size=%zu\n",
           start, end, size);
   ```

3. **Dump and Inspect DWARF**
   ```bash
   # Build with debug symbols
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   
   # Dump eh_frame
   llvm-dwarfdump --eh-frame libChakraCore.dylib | tee dwarf_dump.txt
   
   # Verify:
   # - CIE exists with ARM64 registers
   # - FDEs are generated for functions
   # - CFI directives match prolog instructions
   ```

4. **Test Minimal Case**
   ```javascript
   // test_minimal_unwind.js
   function throws() { throw new Error("test"); }
   function catches() {
       try { throws(); }
       catch (e) { print("SUCCESS"); }
   }
   catches();
   ```
   
   Run with: `./ch test_minimal_unwind.js`
   
   Expected: Prints "SUCCESS"
   Actual: Process terminates

**Success Criteria:**
- DWARF is being generated
- `__REGISTER_FRAME` is being called
- DWARF structure looks correct in dwarfdump

**If Successful:** Proceed to Phase 2
**If Failed:** Fix DWARF generation before continuing

---

### Phase 2: Fix DWARF Issues (3-5 days)

**Goal:** Make DWARF generation correct and complete

**Known Issues to Fix:**

1. **CFA Tracking**
   ```cpp
   // In PrologEncoder.cpp
   // Ensure cfaWordOffset accurately tracks stack depth
   // ARM64 uses 16-byte alignment - verify calculations
   
   // When allocating stack:
   cfaWordOffset += (allocSize / MachPtr);  // Verify this is correct
   
   // When setting up FP:
   fde->cfi_def_cfa_register(DWARF_RegNum[RegFP]);  // Are we emitting this?
   ```

2. **Individual STR Support**
   ```cpp
   // In PrologEncoderMD::GetOp()
   // Ensure individual STR (Apple Silicon) is properly classified
   
   case Js::OpCode::STR:
       // Extract register and offset
       // Return appropriate opcode
       return OP_SAVE_REG;
   ```

3. **Register Offset Calculations**
   ```cpp
   // In PrologEncoder::EncodeInstr()
   // For STR x19, [sp, #64]:
   // CFI should be: .cfi_offset x19, (64 - currentCFAOffset)
   
   int32_t cfaRelativeOffset = offsetFromSP - currentCFAOffset;
   fde->cfi_offset(GetDwarfRegNum(reg), cfaRelativeOffset);
   ```

4. **Frame Pointer Transition**
   ```cpp
   // After: ADD x29, sp, #160
   // Need: .cfi_def_cfa_register x29
   // Then: .cfi_def_cfa_offset (totalStackSize - 160)
   
   case PrologEncoderMD::OP_SET_FP:
       fde->cfi_def_cfa_register(DWARF_RegNum[RegFP]);
       // Update CFA offset calculation
       break;
   ```

**Testing Strategy:**
- Add test for each prolog pattern
- Verify CFI with dwarfdump after each fix
- Test exception handling after each change
- Use lldb to inspect stack frames

**Success Criteria:**
- dwarfdump shows complete and correct CFI
- Simple cross-function exception test passes
- Stack traces in lldb show correct frames

---

### Phase 3: Fix Interpreter Exceptions (3-7 days)

**Goal:** Understand and fix why interpreter-only mode fails

**Investigation Steps:**

1. **Compare with x64 Unix**
   ```bash
   # Does x64 Unix ChakraCore handle cross-function exceptions?
   # If yes, what's different on ARM64?
   # Check PAL exception handling code
   ```

2. **Trace Exception Flow**
   ```bash
   lldb -- ./ch test_cross_function.js
   (lldb) break set -n __cxa_throw
   (lldb) break set -n _Unwind_RaiseException
   (lldb) run
   # When it breaks, examine:
   (lldb) bt        # Stack trace
   (lldb) reg read  # Register state
   (lldb) disassemble  # What code is running
   ```

3. **Verify JavascriptException**
   ```cpp
   // In Exceptions/JavascriptException.h
   class JavascriptException : public std::exception {
       // Is this properly set up?
       // Does it have correct type_info?
       // Is what() implemented?
   };
   
   // Test with simple C++ throw:
   try {
       throw Js::JavascriptException(...);
   } catch (const std::exception& e) {
       // Does this catch it?
   }
   ```

4. **Check PAL Signal Handler**
   ```cpp
   // In pal/src/exception/signal.cpp
   // Ensure ARM64 signal handling is implemented
   // Check if SIGSEGV/SIGABRT handlers are registered
   // Verify exception filter chain
   ```

5. **Compare ARM64 Windows vs Unix**
   ```cpp
   // Windows ARM64 uses structured exception handling (SEH)
   // Unix ARM64 uses Itanium C++ ABI (table-based)
   // Are both paths implemented in ChakraCore?
   // Check #ifdef _WIN32 vs Unix in exception code
   ```

**Potential Fixes:**

1. **Missing PAL Implementation**
   - Port x64 Unix exception handling to ARM64
   - Add ARM64-specific signal handlers
   - Implement ARM64 context conversion

2. **JavascriptException Integration**
   - Ensure proper RTTI for exception type
   - Fix exception propagation through native code
   - Verify exception filter registration

3. **Thunk Issues**
   - `arm64_CallEhFrame` helper may be Windows-specific
   - Need Unix equivalent or workaround
   - May need to restructure exception handling

**Success Criteria:**
- Interpreter-only mode can handle cross-function exceptions
- Basic JavaScript `try/catch` works without JIT
- Foundation is solid before adding JIT complexity

---

### Phase 4: Split Epilog and Try/Catch Support (2-4 days)

**Goal:** Support ChakraCore's special exception handling model

**Requirements:**

1. **Functions with `HasTry()` Generate Special Code**
   ```cpp
   // In Lowerer.cpp
   if (this->m_func->HasTry()) {
       // Save ALL callee-saved registers (not just used ones)
       // Home ALL parameter registers
       // Generate EH epilog label
   }
   ```

2. **Generate DWARF for Both Epilog Paths**
   - Normal epilog: deallocate + restore + return
   - EH epilog: restore + return (skip deallocation)
   
   **Options:**
   - Single FDE with CFI for both paths
   - Separate FDE for EH epilog
   - Use LSDA (Language Specific Data Area) for landing pads

3. **Integrate with arm64_CallEhFrame**
   ```assembly
   # From arm64_CallEhFrame.S
   # Creates synthetic frame to resume in catch block
   # Relies on parent function's unwind metadata
   
   # Need to ensure:
   # - Parent function's DWARF is correct
   # - Stack layout matches expectations
   # - Register saves are complete
   ```

**Testing:**
```javascript
// Test nested try/catch
function test() {
    try {
        try {
            throw new Error("inner");
        } catch (inner) {
            print("Caught inner");
            throw new Error("outer");
        }
    } catch (outer) {
        print("Caught outer");
    }
}

// Test finally
function testFinally() {
    try {
        throw new Error("test");
    } finally {
        print("Finally ran");
    }
}
```

**Success Criteria:**
- Nested try/catch works correctly
- Finally blocks execute properly
- Exception state is maintained across handlers

---

### Phase 5: Integration and Performance (1-2 weeks)

**Goal:** Ensure robustness and acceptable performance

**Tasks:**

1. **Comprehensive Testing**
   - Run full ChakraCore test suite
   - Test real-world JavaScript code
   - Stress test with complex exception patterns
   - Verify garbage collection interaction

2. **Performance Validation**
   - Measure overhead of individual STR vs STP
   - Profile exception handling performance
   - Optimize hot paths
   - Consider caching DWARF structures

3. **Edge Cases**
   - Mixed JIT/interpreter call stacks
   - Exceptions during JIT compilation
   - Stack overflow handling
   - Debugger interaction

4. **Documentation**
   - Update KNOWN_LIMITATIONS.md
   - Document Apple Silicon constraints
   - Create troubleshooting guide
   - Add developer documentation

**Success Criteria:**
- All tests pass
- Performance acceptable (within 20% of x64)
- No known crash cases
- Ready for production use

---

## Alternative Approaches

### If DWARF Proves Too Complex

#### Option A: Disable Try/Catch in JIT

**Approach:**
- Detect functions with `HasTry()`
- Fall back to interpreter for those functions
- JIT only functions without exception handling

**Pros:**
- Avoids the DWARF complexity entirely
- Simpler implementation
- Might be acceptable for some workloads

**Cons:**
- Significant performance impact
- Many real-world functions use try/catch
- Defeats much of the purpose of JIT

**Verdict:** Last resort only

#### Option B: Compact Unwind Info

**Approach:**
- Use Apple's compact unwind format instead of DWARF
- Native macOS format, potentially more efficient

**Pros:**
- Native to macOS
- May be more efficient
- Better integrated with Apple tools

**Cons:**
- Much more complex to generate
- Less portable
- No existing ChakraCore code
- Would need to implement from scratch

**Verdict:** Future optimization after DWARF works

#### Option C: Frame Pointer Only

**Approach:**
- Rely solely on frame pointers for unwinding
- No metadata required

**Pros:**
- Simpler code generation
- Less metadata to manage

**Cons:**
- Apple's unwinder **requires** metadata
- Already tested - doesn't work
- Not a viable solution

**Verdict:** Not viable

---

## Critical Dependencies

### External Factors

1. **ChakraCore Upstream**
   - ARM64 Unix support may be incomplete upstream
   - May need to contribute fixes back
   - Check if others have solved this

2. **Apple Platform**
   - libunwind behavior may change with macOS updates
   - JIT restrictions may evolve
   - Need to track Apple documentation

3. **Build Toolchain**
   - CMake support for eh_frame generation
   - Linker behavior with JIT code
   - Code signing requirements

### Internal Dependencies

1. **PAL (Platform Abstraction Layer)**
   - May need ARM64 Unix implementation
   - Exception filter registration
   - Signal handler setup

2. **Memory Management**
   - JIT memory allocation with MAP_JIT
   - W^X state transitions
   - Code signing integration

3. **Register Allocation**
   - Avoid x18 (platform reserved)
   - Respect calling convention
   - Track callee-saved register usage

---

## Resource Estimates

### Time Estimates (Single Engineer)

| Phase | Optimistic | Realistic | Pessimistic |
|-------|-----------|-----------|-------------|
| Phase 1: Verify DWARF | 1 day | 2 days | 3 days |
| Phase 2: Fix DWARF | 3 days | 5 days | 7 days |
| Phase 3: Fix Interpreter | 3 days | 7 days | 14 days |
| Phase 4: Split Epilog | 2 days | 4 days | 7 days |
| Phase 5: Integration | 5 days | 10 days | 14 days |
| **Total** | **14 days** | **28 days** | **45 days** |

**Note:** Phase 3 (interpreter exceptions) is the highest risk. If the root cause is deep in the PAL or requires architectural changes, it could take significantly longer.

### Skills Required

- **Deep DWARF/CFI knowledge** - Understanding Call Frame Information
- **ARM64 assembly expertise** - Reading and writing ARM64 code
- **C++ exception internals** - How libunwind and __cxa_throw work
- **ChakraCore internals** - JIT, interpreter, exception model
- **macOS low-level debugging** - lldb, dwarfdump, code signing
- **Perseverance** - This is difficult, frustrating work

---

## Success Metrics

### Minimum Viable Product

- [ ] Cross-function exceptions work in JIT code
- [ ] Cross-function exceptions work in interpreter
- [ ] Basic try/catch/finally functional
- [ ] No known crash cases with test suite
- [ ] Performance within 2x of x64

### Full Success

- [ ] All ChakraCore tests pass
- [ ] Real-world JavaScript works
- [ ] Nested exceptions handled correctly
- [ ] Performance within 20% of x64
- [ ] Debugger integration works
- [ ] Code is maintainable and documented

### Production Ready

- [ ] All tests pass consistently
- [ ] Performance benchmarks met
- [ ] Edge cases handled
- [ ] Documentation complete
- [ ] Ready for public release

---

## Known Limitations (Current)

### What Doesn't Work

- ❌ Cross-function exceptions (JIT)
- ❌ Cross-function exceptions (interpreter)
- ❌ Built-in functions that throw
- ❌ Complex try/catch patterns
- ❌ Finally blocks across functions
- ❌ Nested exception handlers

### What Works

- ✅ Basic code generation
- ✅ Simple functions without exceptions
- ✅ Same-function try/catch
- ✅ Math and simple operations
- ✅ Function calls (no exceptions)

### Workarounds

Currently, the only workaround is:
- Don't use try/catch
- Don't call functions that might throw
- Essentially: don't use it for real JavaScript

**This makes the JIT unusable for production.**

---

## References

### ChakraCore Resources

- [PR #4182](https://github.com/chakra-core/ChakraCore/pull/4182) - ARM64 exception handling work
- [tcare/ChakraCore arm64 branch](https://github.com/tcare/ChakraCore/tree/arm64) - External ARM64 port with CFI macros
- `lib/Backend/arm64/arm64_CallEhFrame.S` - Exception frame helper (Windows)
- `pal/inc/unixasmmacrosarm64.inc` - CFI-emitting assembly macros

### Apple Documentation

- [Writing ARM64 Code for Apple Platforms](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms)
- [ARM64 Exception Handling](https://developer.apple.com/library/archive/documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARM64FunctionCallingConventions.html)
- Hardened Runtime and JIT entitlements

### Standards and Specifications

- **DWARF 4 Standard** - Section 6.4 (Call Frame Information)
- **ARM64 ELF ABI** - Exception handling supplement
- **Itanium C++ ABI** - Exception handling specification
- **ARM64 DWARF Register Numbering** - Official ABI specification

### External Resources

- [LLVM ARM64 Backend](https://github.com/llvm/llvm-project/tree/main/llvm/lib/Target/AArch64) - Reference implementation
- [GCC ARM64](https://github.com/gcc-mirror/gcc/tree/master/gcc/config/aarch64) - Prolog generation
- [libunwind documentation](https://github.com/libunwind/libunwind) - Unwinding internals

### Tools

- `llvm-dwarfdump` - Inspect DWARF information
- `otool -l` - Check Mach-O sections
- `lldb` - Debugging and stack inspection
- `objdump --dwarf=frames` - Alternative DWARF viewer

---

## Conclusion

The Apple Silicon JIT port has made significant progress in code generation and constraint handling, but **exception handling is completely broken**. This is a critical blocker for production use.

### Key Insights

1. **Infrastructure Exists** - DWARF generation code is written and compiles
2. **Not Tested** - We don't know if it works or is even being executed
3. **Deeper Issue** - Even interpreter mode fails, suggesting runtime integration problems
4. **Clear Path** - Five phases of work with measurable milestones
5. **High Risk** - Phase 3 (interpreter exceptions) could uncover fundamental issues

### Recommended Action

1. **Start with Phase 1** - Verify DWARF is being generated (1-2 days)
2. **Assess Results** - If DWARF is good, proceed to Phase 2
3. **If DWARF is Bad** - Fix generation before moving forward
4. **Phase 3 is Critical** - Fixing interpreter exceptions is highest priority
5. **Don't Skip Steps** - Each phase builds on the previous

### Final Note

This is **hard work** that requires expertise in multiple domains. The person tackling this needs:
- Deep understanding of exception handling internals
- ARM64 assembly proficiency
- Ability to read and debug complex C++ code
- Patience to work through obscure runtime issues
- Access to Apple Silicon hardware for testing

**Estimate:** 4-6 weeks for an experienced engineer, potentially longer if unexpected issues arise.

---

**Document Status:** Complete - Ready for implementation
**Next Action:** Begin Phase 1 - Verify DWARF Generation
**Owner:** TBD
**Priority:** CRITICAL