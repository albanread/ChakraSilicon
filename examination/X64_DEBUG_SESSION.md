# x64 Debug Session - Observations

**Date:** 2024 (Current Session)  
**Goal:** Understand how the WORKING x64 implementation handles JIT→INT calls  
**Method:** Debug x64 build under Rosetta with LLDB

---

## Session Setup

**Binary:** `./dist/chjitx64/ch` (x64 running under Rosetta on Apple Silicon)  
**Test:** `examination/test_jit_to_int.js`  
**Debugger:** LLDB with `examination/research_x64.lldb`

---

## Test Case Being Debugged

```javascript
function testMathMax() {
    var result = Math.max(42, 17);
    return result;
}

// Warm up to trigger JIT compilation
for (var i = 0; i < 10000; i++) {
    testMathMax();
}

// Final execution (should be JIT→INT)
var result1 = testMathMax();
```

**Expected:** JIT-compiled code calls Math.max (interpreter-mode built-in)

---

## Observation Point 1: InterpreterHelper Entry

### When Breakpoint Hits

**Location in code:** `InterpreterStackFrame::InterpreterHelper`

### Register Values (x64 SysV ABI)

```
RDI (arg 0) = 
RSI (arg 1) = 
RDX (arg 2) = 
RCX (arg 3) = 
R8  (arg 4) = 
R9  (arg 5) = 
RAX         = 
RBX         = 
RBP         = 
RSP         = 
```

**Analysis:**
- RDI should be: ScriptFunction* (function object)
- RSI should be: CallInfo structure
- What do we actually see?

### Stack Contents at RSP

```
[RSP + 0x00] = 
[RSP + 0x08] = 
[RSP + 0x10] = 
[RSP + 0x18] = 
[RSP + 0x20] = 
[RSP + 0x28] = 
[RSP + 0x30] = 
[RSP + 0x38] = 
```

**Analysis:**
- What's on the stack?
- Return address?
- Saved parameters?
- Layout matches ARGUMENTS macro expectations?

### Disassembly at Entry

```asm
; Paste disassembly here
```

**Key instructions:**
- How does it access parameters?
- What stack offsets?

### Backtrace

```
#0 InterpreterStackFrame::InterpreterHelper
#1 
#2 
#3 
#4 
#5 
#6 
#7 
#8 
```

**Analysis:**
- How did we get here?
- Is there a thunk in the chain?
- What's between JIT code and InterpreterHelper?

---

## Observation Point 2: The Thunk

### Static Thunk (if visible in backtrace)

**Address:** 

**Disassembly:**
```asm
; Paste thunk disassembly here
```

**What it does:**
1. 
2. 
3. 
4. 

### Dynamic Thunk (if we can find it)

**Address:** 

**How to find it:**
- Look at X3/RDX register in static thunk
- Or search backtrace

**Disassembly:**
```asm
; Paste dynamic thunk disassembly here
```

---

## ARGUMENTS Macro in Action

### How Parameters Are Accessed

When InterpreterHelper calls:
```cpp
ARGUMENTS(args, callInfo);
```

**Macro expansion (for x64):**
```cpp
Js::Var* _argsVarArray = _get_va(_AddressOfReturnAddress(), 2);
```

**Where _AddressOfReturnAddress points:**

**What _get_va returns:**

**Where args[0], args[1], etc. are found:**

---

## Call Chain Diagram

Based on observations, draw the complete call chain:

```
JIT-compiled testMathMax()
    | (How does it call? What instruction?)
    ↓
[Static Thunk?]
    | (What does it do?)
    ↓
[Dynamic Thunk?]
    | (What does it do?)
    ↓
InterpreterHelper
    | (Uses ARGUMENTS macro)
    ↓
Math.max built-in implementation
```

**Fill in the blanks:**
- 
- 
- 

---

## Key Findings

### Finding #1: Parameter Passing

**How parameters flow from JIT to built-in:**


### Finding #2: Stack Layout

**Actual stack layout at InterpreterHelper:**
```
[RetAddr] [param1] [param2] [...]
```

**Does it match the comment in Arguments.h?**
> "At entry of JavascriptMethod the stack layout is: [Return Address] [function] [callInfo] [arg0] [arg1] ..."

**Match?** YES / NO / PARTIAL

**Differences:**


### Finding #3: The Thunk's Role

**What the thunk actually does:**


### Finding #4: Register Usage

**Which registers are used for what:**
- Function object: 
- CallInfo: 
- Arguments: 
- Return address tracking: 

---

## Comparison with ARM64 Source

### x64 Source Code vs. Observed Behavior

**Does source match what we see?** YES / NO

**Discrepancies:**


### ARM64 Source Code Differences

**Key differences between x64 and ARM64 thunk code:**
1. 
2. 
3. 

**Which differences matter for Darwin?**


---

## Questions Answered

### ✅ Q1: What values are in registers at thunk entry?

**Answer:**


### ✅ Q2: What does the thunk do with parameters?

**Answer:**


### ✅ Q3: Where does the ARGUMENTS macro find parameters?

**Answer:**


### ✅ Q4: What is the stack frame layout?

**Answer:**


### ✅ Q5: How does varargs calling convention work?

**Answer:**


### ✅ Q6: What does the dynamic thunk do?

**Answer:**


---

## Hypotheses for ARM64 Fix

Based on x64 observations, the ARM64 fix should:

1. 

2. 

3. 

**Confidence level:** 

**Why this should work:**


---

## Next Steps

1. [ ] Record all observations above
2. [ ] Compare with ARM64 source code
3. [ ] Identify specific differences
4. [ ] Design targeted fix
5. [ ] Implement fix
6. [ ] Test on ARM64

---

## Notes / Raw Observations

(Record anything else you notice during debugging)

-
-
-

---

**Status:** Research in progress - filling in observations from actual debugging session