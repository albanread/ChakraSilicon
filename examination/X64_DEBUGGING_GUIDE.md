# x64 JIT→INT Interface Debugging Guide

**Purpose:** Learn how the working x64 implementation handles JIT-to-interpreter transitions, so we can fix the broken ARM64 implementation.

**Strategy:** Run the x64 build under Rosetta emulation, debug it, and document exactly how the thunk works.

---

## Overview

We have a **working** x64 Mac JIT implementation and a **broken** ARM64 Mac JIT implementation. The x64 version successfully handles JIT→INT (JIT code calling interpreter-mode built-ins), while ARM64 hangs.

By studying the x64 implementation under Rosetta, we can:
1. Understand the correct thunk behavior
2. See how parameters are passed
3. Learn what the ARGUMENTS macro expects
4. Document the stack layout
5. Apply these insights to fix ARM64

---

## Prerequisites

### Build x64 Binary (Already Done!)

```bash
# x64 binary built under Rosetta
./dist/chjitx64/ch
./dist/chjitx64/libChakraCore.dylib
```

Verify it works:
```bash
./dist/chjitx64/ch -version
# Should output: ch version 1.13.0.0-beta
```

### Test Case

Use `examination/test_jit_to_int.js` which triggers various JIT→INT calls:
```bash
./dist/chjitx64/ch examination/test_jit_to_int.js
# Should output: All tests PASS
```

---

## Debugging Session: Quick Start

### Method 1: Using LLDB Script (Automated)

```bash
lldb ./dist/chjitx64/ch
(lldb) command source examination/debug_x64_thunk.lldb
(lldb) run examination/test_jit_to_int.js
```

The script will automatically:
- Set breakpoints at key locations
- Capture register and stack state
- Disassemble relevant code
- Log information for analysis

### Method 2: Manual Debugging (Detailed Control)

```bash
lldb ./dist/chjitx64/ch
```

Then follow the steps below.

---

## Step-by-Step Debugging

### Step 1: Set Breakpoints

```lldb
# Critical breakpoint - where JIT transitions to interpreter
(lldb) breakpoint set -n "InterpreterStackFrame::InterpreterHelper"

# Optional - built-in function entry
(lldb) breakpoint set -r "Math.*Max"

# Optional - thunk generation (to capture generated code)
(lldb) breakpoint set -r "InterpreterThunkEmitter"
```

### Step 2: Run the Test

```lldb
(lldb) run examination/test_jit_to_int.js
```

### Step 3: When Breakpoint Hits

The first JIT→INT call will trigger `InterpreterHelper`. Now we examine state:

#### A. Capture Register State

```lldb
(lldb) register read

# Key registers for x64 calling convention:
# rdi = 1st parameter (typically: RecyclableObject* function)
# rsi = 2nd parameter (typically: CallInfo)
# rdx = 3rd parameter (typically: varargs start)
# rcx = 4th parameter
# r8  = 5th parameter
# r9  = 6th parameter
# Additional args on stack

(lldb) register read rdi rsi rdx rcx r8 r9
```

**Document what you see:**
- What type of value is in each register?
- Are they pointers? Integers? Structures?
- Where do they point to?

#### B. Examine Stack Layout

```lldb
(lldb) memory read -f x -c 32 $rsp

# Look for:
# - Return addresses
# - Saved frame pointer
# - Parameter values
# - Local variables
```

**Document:**
- Stack frame size
- Offsets of saved values
- Where parameters end up on stack

#### C. Disassemble the Thunk

```lldb
(lldb) disassemble -b -c 50
```

Look for the thunk code that was just executed. It will show:
- Prologue (stack frame setup)
- Parameter handling
- Jump to dynamic thunk or interpreter

**Document:**
- Exact assembly instructions
- Stack allocation size
- Register saves/restores
- How it accesses parameters

#### D. Examine Backtrace

```lldb
(lldb) bt 10
```

This shows the call chain:
```
#0 InterpreterStackFrame::InterpreterHelper
#1 <generated thunk code>
#2 <JIT-compiled JavaScript function>
#3 ... more JS call stack
```

**Document:**
- What's on the stack between JIT and interpreter?
- How many frames?
- What's the return path?

### Step 4: Step Through Parameter Access

```lldb
# Step into the interpreter helper
(lldb) step

# Watch how it accesses parameters
# The ARGUMENTS macro will compute addresses based on stack layout
```

**Key observations:**
- How does it find the CallInfo?
- How does it find individual arguments?
- What assumptions does it make about stack layout?

### Step 5: Compare with Source Code

While debugging, have these files open:

1. **Thunk generation:**
   ```
   ChakraCore/lib/Runtime/Language/InterpreterThunkEmitter.cpp
   ```
   Look for `x64` or `amd64` specific code.

2. **Arguments macro:**
   ```
   ChakraCore/lib/Runtime/Language/Arguments.h
   ```
   Find the `ARGUMENTS` macro definition.

3. **Interpreter helper:**
   ```
   ChakraCore/lib/Runtime/Language/InterpreterStackFrame.cpp
   ```
   The `InterpreterHelper` function.

**Cross-reference:**
- Does the generated thunk match what the source says it should do?
- Does the stack layout match what ARGUMENTS expects?
- Are there any platform-specific differences?

---

## Key Information to Document

Create a document with these findings:

### 1. x64 Calling Convention (as observed)

```
Function: InterpreterStackFrame::InterpreterHelper
Parameters:
  rdi (arg0) = <document what you see>
  rsi (arg1) = <document what you see>
  rdx (arg2) = <document what you see>
  rcx (arg3) = <document what you see>
  r8  (arg4) = <document what you see>
  r9  (arg5) = <document what you see>
  [rsp+8]    = <additional args on stack?>
```

### 2. Thunk Assembly (actual disassembly)

```asm
; Document the exact instructions you see
; Example format:
push   rbp
mov    rbp, rsp
sub    rsp, 0x??     ; <-- How much stack allocated?
mov    [rbp-0x??], rdi  ; <-- Where are params saved?
; ... rest of thunk
```

### 3. Stack Frame Layout (as observed)

```
Stack at InterpreterHelper entry:
[rbp+0x18] = <what's here?>
[rbp+0x10] = <what's here?>
[rbp+0x08] = Return address to JIT code
[rbp+0x00] = Saved RBP (frame pointer)
[rbp-0x08] = <local or saved register?>
[rbp-0x10] = <local or saved register?>
...
```

### 4. ARGUMENTS Macro Expectations

After seeing how parameters are accessed:
```
ARGUMENTS macro expects:
- CallInfo at: <address calculation formula>
- Arg[0] at: <address calculation formula>
- Arg[1] at: <address calculation formula>
- Varargs start: <where?>
```

### 5. Dynamic Thunk (if found)

```
Dynamic thunk address: <captured from register>
What it does: <document behavior>
Parameters it expects: <document>
How static thunk calls it: <document>
```

---

## Advanced Analysis

### Capture Generated Thunk Code

The thunk is generated at runtime. To save it for detailed analysis:

```lldb
# Break at thunk generation
(lldb) breakpoint set -n "InterpreterThunkEmitter::Emit"
(lldb) run examination/test_jit_to_int.js

# When it hits:
(lldb) register read rax  # or wherever return value is
# This should be the address of generated thunk

# Disassemble and save
(lldb) disassemble -s <address> -c 100
```

Copy the assembly to a file: `examination/x64_thunk_disassembly.asm`

### Use Capstone for Analysis

If you want to analyze the binary thunk:

```python
# Python script using Capstone
from capstone import *

# Dump memory from lldb first
# (lldb) memory read -o thunk.bin -c 256 <address>

md = Cs(CS_ARCH_X86, CS_MODE_64)
with open("thunk.bin", "rb") as f:
    code = f.read()
    for insn in md.disasm(code, 0x0):
        print(f"0x{insn.address:x}:\t{insn.mnemonic}\t{insn.op_str}")
```

### Compare with Windows x64

If you have access to the ChakraCore source history or Windows builds:

```bash
# Search for Windows x64 thunk
grep -A 50 "TARGET_64.*Windows" ChakraCore/lib/Runtime/Language/InterpreterThunkEmitter.cpp
```

Document differences:
- Does Windows use a different calling convention?
- Stack allocation size?
- Parameter handling?

---

## Critical Questions to Answer

By the end of your debugging session, you should be able to answer:

### ✅ Q1: Thunk Prologue
- How much stack space does the thunk allocate?
- What does it save to the stack?
- In what order?

### ✅ Q2: Parameter Handling
- Where do the first 6 parameters end up?
- Are they kept in registers or moved to stack?
- If moved to stack, at what offsets?

### ✅ Q3: Dynamic Thunk
- What is the address of the dynamic thunk?
- What parameters does it expect?
- How does the static thunk prepare for calling it?

### ✅ Q4: ARGUMENTS Macro
- What pointer arithmetic does ARGUMENTS use?
- What base address does it start from?
- What does it assume about stack layout?

### ✅ Q5: Varargs Handling
- How are variadic arguments (like Math.max(a,b,c,...)) handled?
- Where does the thunk put them?
- How does CallInfo communicate the count?

### ✅ Q6: Return Path
- How does the interpreter return to JIT code?
- Is there an epilogue?
- What state must be restored?

---

## Comparison with ARM64

Once you understand x64, create a comparison table:

| Aspect | x64 (Working) | ARM64 (Broken) | Notes |
|--------|---------------|----------------|-------|
| Stack allocation | ? bytes | ? bytes | Same or different? |
| Param save location | ? | ? | Registers vs stack |
| First param (X0/RDI) | ? | ? | What value? |
| Second param (X1/RSI) | ? | ? | What value? |
| CallInfo location | ? | ? | Key for ARGUMENTS |
| Varargs start | ? | ? | Critical |
| Dynamic thunk call | ? | ? | How invoked? |

Fill this in as you debug.

---

## Expected Findings

Based on typical calling conventions, you might find:

### x64 SysV ABI (Darwin)
```
rdi = function object pointer
rsi = CallInfo structure (count + flags)
rdx = pointer to first argument (varargs start)
rcx-r9 = additional fixed params or unused
stack = remaining arguments if > 6 params
```

### ARM64 AAPCS64 (What ARM64 *should* use)
```
x0 = function object pointer
x1 = CallInfo structure
x2 = pointer to first argument (varargs start)
x3-x7 = additional fixed params or unused
stack = remaining arguments if > 8 params
```

**The key insight:** Does the x64 thunk actually follow this? Or something different?

---

## Output Format

Create a file: `examination/X64_FINDINGS.md`

Structure it like:
```markdown
# x64 JIT→INT Thunk Analysis Results

## Session Information
- Date: <date>
- Binary: dist/chjitx64/ch
- Test case: examination/test_jit_to_int.js
- Platform: macOS under Rosetta

## Register State at InterpreterHelper Entry

<paste register dump>

## Stack Layout

<paste stack dump with annotations>

## Thunk Disassembly

<paste disassembly with detailed comments>

## ARGUMENTS Macro Behavior

<document how it accesses parameters>

## Key Insights

<bullet points of critical findings>

## Comparison with ARM64 Source

<what's different in the ARM64 thunk code?>

## Root Cause Hypothesis

Based on x64 findings, the ARM64 hang is likely caused by:
<your analysis>

## Recommended Fix

<proposed changes to ARM64 thunk based on x64 understanding>
```

---

## Troubleshooting

### Issue: Breakpoint Never Hits

**Problem:** `InterpreterStackFrame::InterpreterHelper` breakpoint doesn't trigger.

**Solution:**
- Built-in might be JIT-compiled too. Use `-ForceSerialized` flag:
  ```bash
  lldb -- ./dist/chjitx64/ch -ForceSerialized examination/test_jit_to_int.js
  ```
- Or try breaking earlier:
  ```lldb
  (lldb) breakpoint set -r ".*Thunk.*"
  ```

### Issue: Can't See Generated Code

**Problem:** Thunk code is dynamically generated and not visible.

**Solution:**
- Break on thunk generation:
  ```lldb
  (lldb) breakpoint set -r "EmitCall.*Thunk"
  ```
- Capture the address from return value or register
- Use `disassemble -s <address>`

### Issue: Stack Layout Unclear

**Problem:** Stack dump is confusing.

**Solution:**
- Use frame info:
  ```lldb
  (lldb) frame info
  (lldb) frame variable
  ```
- Examine frame pointer chain:
  ```lldb
  (lldb) memory read -f x -c 1 $rbp  # Saved RBP
  (lldb) memory read -f x -c 1 ($rbp + 8)  # Return address
  ```

### Issue: Symbols Missing

**Problem:** Function names show as addresses.

**Solution:**
- Ensure debug symbols:
  ```bash
  # Rebuild with debug info
  cd build/chjitx64
  cmake ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo
  make
  ```

---

## Next Steps

After completing x64 analysis:

1. **Document findings** in `examination/X64_FINDINGS.md`
2. **Compare with ARM64 source** in `ChakraCore/lib/Runtime/Language/InterpreterThunkEmitter.cpp`
3. **Identify the bug** - what's different in ARM64?
4. **Design the fix** - how to make ARM64 match x64 behavior?
5. **Implement and test** - apply fix, verify with test suite
6. **Update `critical_unknowns.md`** - check off answered questions

---

## Resources

### ChakraCore Source Files
- `lib/Runtime/Language/InterpreterThunkEmitter.cpp` - Thunk generation
- `lib/Runtime/Language/Arguments.h` - ARGUMENTS macro
- `lib/Runtime/Language/InterpreterStackFrame.h` - Stack frame structures
- `lib/Runtime/Language/InterpreterStackFrame.cpp` - InterpreterHelper
- `lib/Backend/AMD64/LowerMD.cpp` - x64 JIT lowering

### External References
- **x64 ABI:** System V AMD64 ABI specification
- **ARM64 ABI:** ARM AAPCS64 specification
- **Apple docs:** macOS ABI documentation
- **LLDB docs:** https://lldb.llvm.org/use/tutorial.html

### Comparison Tools
```bash
# Compare x64 and ARM64 thunk generation
diff -u <(grep -A 100 "TARGET_64" lib/Runtime/Language/InterpreterThunkEmitter.cpp) \
        <(grep -A 100 "TARGET_ARM64" lib/Runtime/Language/InterpreterThunkEmitter.cpp)
```

---

## Success Criteria

You've successfully completed the analysis when you can:

1. ✅ Draw a complete diagram of the x64 call chain
2. ✅ Document exact register and stack state at each transition
3. ✅ Explain how ARGUMENTS macro finds parameters
4. ✅ Identify specific differences in ARM64 implementation
5. ✅ Propose a concrete fix with confidence it will work

**Estimated time:** 2-4 hours of focused debugging

**Payoff:** Understanding to fix ARM64 correctly on first try

---

Good luck! 🔍🐛

Remember: We're not guessing anymore. We're **learning** how it actually works.