# Critical Unknowns - JIT→INT Call Chain Investigation

**Purpose:** Document what we don't understand about the JIT→Interpreter call chain that causes hangs on Apple Silicon ARM64.

**Status:** Research phase - gathering information before attempting fixes

---

## Overview

The Apple Silicon ARM64 JIT has a **P0 blocker**: JIT code calling into interpreter-mode built-in functions causes infinite hangs. We have tried several fixes by copying patterns from other platforms, but these attempts failed because we don't understand the underlying mechanism.

**This document catalogs what we need to research and understand before attempting another fix.**

---

## 🔴 P0 - Blocking Unknowns

These unknowns are directly blocking the JIT→INT call chain from working.

### 1. What is the Dynamic Interpreter Thunk?

**What we know:**
- The static thunk (which we can see and modify) jumps to it via `blr x3`
- The target address is loaded into the X3 register
- This address comes from the `dynamicInterpreterThunk` field in some structure
- It's the actual code that bridges from generated code into the interpreter

**What we DON'T know:**
- **What code does it execute?** Where is the implementation?
- **What parameters does it expect?** Which registers? What stack layout?
- **How does it differ between platforms?** Windows vs Linux vs Darwin?
- **Where is it defined in the codebase?** Which file(s)?
- **What assumptions does it make** about the state left by the static thunk?

**Why this matters:**
The static thunk we're modifying is just a trampoline. The real work happens in the dynamic thunk. If we don't understand what it expects, we're flying blind.

**How to find out:**
```bash
# Search for the implementation
grep -r "dynamicInterpreterThunk" ChakraCore/lib/
grep -r "DynamicThunk" ChakraCore/lib/Runtime/

# Runtime debugging
# - Set breakpoint in static thunk
# - Capture X3 value
# - Disassemble that address
# - Trace what it accesses
```

**Files likely involved:**
- `ChakraCore/lib/Runtime/Language/InterpreterStackFrame.cpp`
- `ChakraCore/lib/Runtime/Language/JavascriptStackWalker.cpp`
- Platform-specific runtime files

---

### 2. Why Did the Windows-Style Fix Fail?

**What we tried:**
Applied the Windows ARM64 thunk pattern to Darwin:

```asm
stp     fp, lr, [sp, #-80]!     # Allocate 80 bytes like Windows
stp     x0, x1, [sp, #16]       # Save all parameter registers
stp     x2, x3, [sp, #32]
stp     x4, x5, [sp, #48]
stp     x6, x7, [sp, #64]
```

**What happened:**
- Different tests started failing (we didn't capture which ones)
- Behavior was worse than the original hang
- We reverted immediately without gathering diagnostics

**What we DON'T know:**
- **Which tests failed specifically?** What code paths were affected?
- **What were the error messages?** Crashes? Assertions? Wrong results?
- **Stack alignment issue?** Darwin requires 16-byte alignment at call sites
- **Offset mismatch?** Does the dynamic thunk expect parameters at different offsets?
- **Breaking ABI assumptions?** Is 80 bytes wrong for Darwin?
- **Register clobbering?** Did we overwrite something the dynamic thunk needs?

**Why this matters:**
This was the most promising fix attempt. Understanding why it failed would give us critical clues about what the dynamic thunk actually expects.

**How to find out:**
Re-apply the fix with full diagnostics:
1. Add extensive logging to the thunk entry/exit
2. Log all register values and stack contents
3. Run the full test suite
4. Capture which tests fail and with what errors
5. **Only then** revert

This will be expensive (takes time to gather) but invaluable for understanding.

---

### 3. ARM64 Darwin Calling Convention for Varargs

**What we know:**
- AAPCS64 (standard ARM64, used on Linux): First 8 args in X0-X7, rest on stack
- X64 Darwin (Intel Mac): Has quirks different from System V ABI
- Our current code uses nullptr padding borrowed from the Linux implementation
- JavaScript built-ins are essentially variadic functions

**What we DON'T know:**
- **Does ARM64 Darwin differ from standard AAPCS64?** 
- **Where do variadic arguments land?** Registers? Stack? Both?
- **Is "stack homing" required?** (Windows requires this - must reserve stack space for register args)
- **Do we need argument duplication?** The old (broken) macro duplicated args to stack
- **What does the ARGUMENTS macro expect to find?** Where does it look for parameters?

**Why this matters:**
If the calling convention differs from what we assume, parameters will be in the wrong place, causing the built-in functions to read garbage.

**How to find out:**
- **Read official Apple ARM64 ABI specification** (search Apple developer docs)
- **Write test C programs** with variadic functions, compile with clang for Darwin ARM64
- **Disassemble and observe** where arguments are placed
- **Check clang codegen** - look at how it generates calls to varargs functions
- **Compare with ChakraCore's ARGUMENTS macro implementation**

**Relevant files:**
- `ChakraCore/lib/Runtime/Language/Arguments.h` - ARGUMENTS macro definition
- `ChakraCore/lib/Runtime/Base/CallInfo.h` - Call information structures

---

## 🟡 P1 - Important Unknowns

These aren't immediately blocking but are critical for a complete understanding.

### 4. ARGUMENTS Macro Implementation

**What it does:**
Provides C++ built-in functions access to JavaScript function parameters passed from generated code.

**Example usage:**
```cpp
Var SomeBuiltIn(RecyclableObject* function, CallInfo callInfo, ...) {
    ARGUMENTS(args, callInfo);
    // args can now access individual parameters
    Var arg0 = args[0];
    // ...
}
```

**What we DON'T know:**
- **How does it compute parameter addresses?** What's the formula?
- **What stack layout does it expect?** Where does it think parameters are?
- **Is there platform-specific implementation?** Different for Windows/Linux/Darwin?
- **Why does it read garbage** with the current thunk?
- **Does it make assumptions** about stack frame layout that we're violating?

**Why this matters:**
Even if we get the calling convention right, if ARGUMENTS expects a different layout, built-ins still won't work.

**How to find out:**
```bash
# Find the macro definition
grep -A 20 "define ARGUMENTS" ChakraCore/lib/Runtime/Language/Arguments.h

# Understand JavascriptCallStackLayout
grep -r "JavascriptCallStackLayout" ChakraCore/lib/Runtime/
```

**Analysis needed:**
- Expand the macro mentally or with preprocessor
- Trace through the pointer arithmetic
- Determine what memory layout it expects
- Cross-reference with what our thunk actually creates

---

### 5. Complete JIT → INT Call Chain

**Partial knowledge:**
We know there's a chain:
```
JIT-generated code → Static Thunk → Dynamic Thunk → Built-in Function
```

**What we DON'T know:**
- **How does the JIT emit the initial call?** What instruction sequence?
- **What does it put in X0-X7?** What do those registers contain?
- **Does it match what the thunk expects?** Or is there already a mismatch?
- **Where does JavascriptCallStackLayout get created?** Who's responsible?
- **Who sets up the stack frame?** JIT? Static thunk? Dynamic thunk?
- **What does each component expect to receive** vs what does it actually get?

**Why this matters:**
We're modifying one piece (static thunk) without understanding the full pipeline. We might be fixing the wrong thing.

**How to find out:**
- **Study JIT code generation:**
  - `ChakraCore/lib/Backend/Lower.cpp` - lowering to machine code
  - `ChakraCore/lib/Backend/ARM64/LowerMD.cpp` - ARM64-specific lowering
  - Search for "InterpreterThunk" or "CallHelper" in JIT code
- **Enable JIT disassembly logs:**
  - Use ChakraCore's built-in JIT trace flags
  - Examine actual generated assembly
- **Use our diagnostics:**
  - `scripts/JitCallDiagnostics.cpp` with Capstone
  - Disassemble generated code at runtime
  - Trace the actual call sequence

**Files to examine:**
- `ChakraCore/lib/Backend/ARM64/EncoderMD.cpp` - ARM64 instruction encoding
- `ChakraCore/lib/Runtime/Language/InterpreterThunkEmitter.cpp` - Thunk generation
- `ChakraCore/lib/Runtime/Language/InterpreterStackFrame.h` - Stack frame structures

---

### 6. Exception Unwinding Relationship

**Context:**
We know there are exception unwinding issues on Apple Silicon ARM64 (documented in `next_steps.md`). That's about cross-function exception handling and DWARF CFI.

**What we DON'T know:**
- **Is the JIT→INT hang related to exception unwinding?** Or completely separate?
- **Does the thunk need to be unwind-safe?** Special DWARF annotations?
- **Could stack layout issues** affect both the hang AND exception unwinding?
- **Are we fighting multiple bugs?** Or is there one root cause?

**Why this matters:**
If these issues are related, we might solve both with one fix. If they're separate, we need to fix them independently.

**How to find out:**
- Compare stack layouts between working and broken cases
- Check if the hang occurs during normal execution or only when exceptions might be involved
- See if adding DWARF annotations to the thunk changes anything
- Test JIT→INT calls in try/catch vs outside try/catch

---

## 🟢 P2 - Nice to Know

These would help understand the bigger picture but aren't critical for fixing the immediate issue.

### 7. Why Windows ARM64 Thunk Works

**Observation:**
The Windows ARM64 implementation allocates 80 bytes and saves X0-X7 to the stack. It works fine.

**What we DON'T know:**
- **Does Windows have different ABI requirements?** Different calling convention?
- **Is the dynamic thunk different on Windows?** Different implementation?
- **Can we just copy Windows approach verbatim?** (We tried and it failed - see #2)
- **What makes Darwin special?** Why doesn't Windows fix work?
- **Are there Windows-specific runtime assumptions** we're missing?

**How to find out:**
- Read Windows ARM64 ABI documentation
- Compare Windows vs Darwin dynamic thunk implementations (if they differ)
- Understand what Windows-specific features might be in play
- Diff the Windows and Darwin thunk code paths

---

### 8. STP vs STR Atomicity Impact

**Context:**
During Phase 2 of ARM64 work, we changed STP/LDP instructions to individual STR/LDR instructions because Apple Silicon prohibits STP/LDP when SP is one of the operands.

**What we DON'T know:**
- **Did this break anything?** We can't run the boxing test to verify
- **Is it related to current issues?** Could using STR instead of STP cause problems?
- **Does atomicity matter?** Are there race conditions we introduced?
- **Should we revert this change?** Or is it fine?

**Why we can't test it:**
Blocked by the JIT→INT hang and exception unwinding issues.

**When to revisit:**
After the main issues are fixed, rerun all tests and check if the STR/LDR changes caused any problems.

---

### 9. Historical Context

**Questions:**
- **Did non-Windows ARM64 thunk ever work?** Was it always broken?
- **Who wrote the original implementation?** What were they thinking?
- **What does git history show?** When was it added? Changed?
- **Was there original design documentation?** Rationale for the approach?
- **Has anyone else hit this issue?** Community reports?

**Why it matters:**
Understanding the original intent might reveal what we're missing or what assumptions have changed.

**How to find out:**
```bash
cd ChakraCore
git log --follow lib/Runtime/Language/InterpreterThunkEmitter.cpp
git blame lib/Runtime/Language/InterpreterThunkEmitter.cpp
# Look for commits, PRs, issues related to ARM64 thunks
```

Check ChakraCore GitHub issues/PRs for ARM64 discussions.

---

## 🎯 Investigation Strategy

### Immediate Actions (Next 2-4 Hours)

**Priority 1: Find the Dynamic Thunk**
```bash
cd ChakraCore
grep -r "dynamicInterpreterThunk" lib/
grep -r "DynamicThunk" lib/Runtime/
grep -r "DynamicInterpreter" lib/Runtime/
```

Once found:
- Read the implementation
- Document what it expects (parameters, stack layout)
- Compare Windows vs Linux vs Darwin versions

**Priority 2: Add Thunk Entry/Exit Logging**

Modify `InterpreterThunkEmitter.cpp` to add diagnostic logging at thunk entry:
```cpp
// Pseudocode - emit ARM64 assembly to log state
// Save registers
// Call fprintf(stderr, "Thunk: X0=%p X1=%p ... X7=%p SP=%p\n", ...)
// Restore registers
// Continue with original thunk code
```

**Priority 3: Run Minimal Test with Logging**
```bash
./chjit test_call_patterns.js 2>&1 | tee thunk_trace.log
```

Analyze the log to see what values are actually in registers and on the stack.

---

### Short Term (Next 1-2 Days)

**Research Task 1: Read Apple ARM64 ABI**
- Download from Apple developer documentation
- Focus on:
  - Parameter passing (registers vs stack)
  - Variadic functions
  - Stack alignment requirements
  - Any Darwin-specific quirks
- Document findings

**Research Task 2: Analyze ARGUMENTS Macro**
- Expand the macro definition
- Trace through the pointer arithmetic
- Determine expected memory layout
- Compare with what our thunk provides

**Research Task 3: Trace JIT Code Generation**
- Enable JIT disassembly logs
- Examine generated call sequences
- Document what JIT puts in registers before calling thunk
- Verify it matches thunk expectations

**Research Task 4: Re-apply Windows Fix with Full Diagnostics**

Only attempt this after completing above research:
1. Create a feature branch
2. Apply Windows-style thunk fix
3. Add extensive logging (before/after thunk, parameter values, etc.)
4. Run **entire** test suite
5. Capture **all** failures with:
   - Test names
   - Error messages
   - Stack traces
   - Log output
6. Document findings thoroughly
7. Revert (unless it works!)

This is expensive but will provide critical data.

---

### Medium Term (Next 3-5 Days)

**Analysis Task 1: Complete Call Chain Documentation**
Document the full pipeline:
- JIT code generation → what it emits
- Static thunk → what it does
- Dynamic thunk → what it expects
- Built-in function → what it receives
- Parameter flow through each stage

**Analysis Task 2: Platform Comparison**
Create a detailed comparison table:
- Windows ARM64 implementation
- Linux ARM64 implementation  
- Darwin ARM64 implementation (current/broken)
- Differences in each component
- Why each difference exists

**Analysis Task 3: Correlation with Exception Issues**
- Test JIT→INT calls inside vs outside try/catch
- Check if unwinding issues are related
- Determine if fixes need to be coordinated

**Design Task: Proper Fix Design**
Based on all gathered information:
- Design the correct thunk implementation
- Document why each part is needed
- Predict what will happen
- Identify test cases to validate
- Create implementation plan

---

## 🚨 Warning Signs

**Signs we're still guessing (BAD):**
- ❌ "Let's try X and see what happens"
- ❌ Applying fixes without understanding why
- ❌ Reverting failures without capturing diagnostics
- ❌ Cargo-culting other platforms blindly
- ❌ Ignoring previous failed attempts

**Signs we're on track (GOOD):**
- ✅ "We understand X because we traced through the code and found..."
- ✅ Documenting expected vs actual behavior
- ✅ Capturing detailed diagnostics before reverting
- ✅ Understanding root cause before attempting fix
- ✅ Systematic investigation with measurable progress

---

## 📊 Current Knowledge Confidence Levels

| Topic | Confidence | What We Need |
|-------|-----------|--------------|
| JIT generates valid code | 🟢 High | None - verified working |
| INT→INT calls | 🟢 High | None - works perfectly |
| INT→JIT calls | 🟢 High | None - fixed and working |
| **JIT→INT calls** | 🔴 **Zero** | **EVERYTHING - total mystery** |
| Static thunk structure | 🟡 Medium | Understand what it should do |
| **Dynamic thunk** | 🔴 **Zero** | **Find it, read it, understand it** |
| **Darwin ARM64 varargs ABI** | 🔴 **Zero** | **Read Apple documentation** |
| ARGUMENTS macro | 🔴 Low | Analyze implementation |
| **Why Windows fix failed** | 🔴 **Zero** | **Gather diagnostics** |
| Exception unwinding relation | 🟡 Medium | Test correlation |
| JIT call emission | 🟡 Medium | Trace code generation |
| Complete call chain | 🔴 Low | Document end-to-end |

**Red items are P0 blockers.**

---

## 💡 Key Insight

**The problem is not lack of attempted fixes.**

**The problem is lack of understanding.**

We have tried multiple fixes by copying patterns from other platforms. They failed because we don't understand what we're fixing.

**The solution path is:**

1. **Research** until we understand the mechanism
2. **Document** what we learn
3. **Design** the correct fix based on understanding
4. **Implement** with confidence
5. **Validate** against expectations

**NOT:**

1. ~~Try random fix~~
2. ~~See if it works~~
3. ~~If not, try another random fix~~
4. ~~Repeat until something sticks~~

---

## 🎯 Success Criteria

**We are ready to attempt a proper fix when we can answer:**

1. ✅ **What code does `blr x3` jump to?** (Find and read the dynamic thunk)
2. ✅ **What parameters does it expect and where?** (Registers? Stack offsets?)
3. ✅ **What does ARGUMENTS macro do with those parameters?** (Memory layout?)
4. ✅ **What does Darwin ARM64 ABI specify for varargs?** (Apple docs)
5. ✅ **Why did Windows-style fix fail specifically?** (Diagnostic data)
6. ✅ **What stack layout satisfies all components?** (Complete understanding)

**Current status:** 0/6 answered

**Estimated research time:** 8-24 hours of focused investigation

**Payoff:** One correct fix instead of continued trial-and-error

---

## 📋 Research Checklist

Track progress on understanding:

### Dynamic Thunk
- [ ] Located in codebase (which file?)
- [ ] Implementation read and understood
- [ ] Parameters documented (registers + stack)
- [ ] Platform differences documented
- [ ] Expectations documented

### Calling Convention
- [ ] Apple ARM64 ABI documentation read
- [ ] Variadic function behavior documented
- [ ] Stack homing requirements documented
- [ ] Differences from AAPCS64 documented
- [ ] Test programs written and analyzed

### ARGUMENTS Macro
- [ ] Macro definition expanded
- [ ] Pointer arithmetic traced
- [ ] Expected memory layout documented
- [ ] Platform-specific variants identified
- [ ] Compatibility with thunk verified

### JIT Code Generation
- [ ] Call emission code located
- [ ] Generated assembly examined
- [ ] Register usage documented
- [ ] Stack setup documented
- [ ] Matches thunk expectations verified

### Windows Fix Failure
- [ ] Fix re-applied in test branch
- [ ] Full diagnostics added
- [ ] All test failures captured
- [ ] Error messages documented
- [ ] Root cause identified

### Call Chain
- [ ] Each component documented
- [ ] Data flow traced end-to-end
- [ ] Assumptions identified
- [ ] Mismatches found
- [ ] Complete picture understood

---

## Next Steps

**DO THIS FIRST:**
1. Search for and read the dynamic thunk implementation
2. Read Apple ARM64 ABI documentation
3. Add diagnostic logging to the thunk

**THEN:**
4. Gather data from test runs
5. Analyze ARGUMENTS macro
6. Trace JIT code generation

**FINALLY:**
7. Re-apply Windows fix with full diagnostics
8. Design proper fix based on complete understanding
9. Implement and validate

**DO NOT:**
- Try more random fixes
- Copy patterns without understanding
- Skip diagnostic gathering
- Assume without verifying

---

## References for Research

### ChakraCore Source Files
- `lib/Runtime/Language/InterpreterThunkEmitter.cpp` - Thunk generation
- `lib/Runtime/Language/Arguments.h` - ARGUMENTS macro
- `lib/Runtime/Language/InterpreterStackFrame.h` - Stack frame structures
- `lib/Backend/ARM64/LowerMD.cpp` - ARM64 JIT lowering
- `lib/Backend/ARM64/EncoderMD.cpp` - ARM64 instruction encoding

### External Documentation
- **Apple ARM64 ABI**: Search "Apple ARM64 ABI" in developer docs
- **AAPCS64**: ARM Architecture Procedure Call Standard (standard ARM64 ABI)
- **ARM64 ISA**: ARM Architecture Reference Manual

### Search Commands
```bash
# Find dynamic thunk
grep -r "dynamicInterpreterThunk\|DynamicThunk" ChakraCore/lib/

# Find ARGUMENTS usage
grep -r "ARGUMENTS(" ChakraCore/lib/Runtime/Library/

# Find JIT call generation
grep -r "InterpreterThunk\|CallHelper" ChakraCore/lib/Backend/

# Git history
git log --follow ChakraCore/lib/Runtime/Language/InterpreterThunkEmitter.cpp
```

---

## Conclusion

This document captures **what we don't know** about the JIT→INT call chain that's causing hangs on Apple Silicon ARM64.

**The path forward is research, not guessing.**

Once we understand these unknowns, the fix will be straightforward. Until then, more random attempts will waste time and potentially make things worse.

**Estimated time to completion:**
- Research phase: 8-24 hours
- Design phase: 2-4 hours  
- Implementation: 2-8 hours
- Validation: 4-8 hours

**Total: 2-5 days of focused work**

Much better than weeks of trial-and-error.