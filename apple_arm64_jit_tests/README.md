# Apple ARM64 JIT Call Bugs - Test Suite

## Overview

This test suite validates the fixes for two related but distinct bugs in ChakraCore's JIT compiler on Apple Silicon (ARM64 with DarwinPCS):

- **Bug A**: CallDirect → C++ variadic `Entry*` helper crashes/TypeErrors
- **Bug B**: JS→JS function calls with 7+ parameters crash or pass incorrect values

## Quick Start

### Prerequisites

1. **Build ChakraCore** on Apple Silicon:
   ```bash
   cd ChakraCore
   ./build.sh --debug    # or --release
   ```

2. **Verify you're on Apple Silicon**:
   ```bash
   uname -m   # Should show: arm64
   uname -s   # Should show: Darwin
   ```

### Run All Tests

```bash
cd apple_arm64_jit_tests
chmod +x run_tests.sh
./run_tests.sh all
```

### Run Specific Tests

```bash
# Bug A only (CallDirect varargs)
./run_tests.sh varargs

# Bug B only (JS→JS 7+ parameters)
./run_tests.sh params

# Quick test (JIT-enabled only, no comparison)
./run_tests.sh quick
```

### Custom Binary Path

```bash
export CHAKRA_BIN=/path/to/your/ch
./run_tests.sh all
```

## Test Files

### test_calldirect_varargs.js (Bug A)

Tests C++ variadic Entry* helper calls from JIT-compiled code.

**Coverage**: 32 test cases including TypedArray methods, Array methods, String methods, RegExp operations, edge cases, and stress tests.

**Expected behavior**: All tests pass without crashes or TypeErrors when JIT is enabled.

### test_js_7plus_params.js (Bug B)

Tests JS-to-JS function calls with 7 or more parameters.

**Coverage**: 30 test cases including various parameter counts, data types, nested calls, constructors, and stress tests.

**Expected behavior**: All tests pass without crashes and all parameter values are correct.

## Expected Results

### After Fix (Success)

```
✓ ALL TESTS PASSED - CallDirect varargs bug is FIXED!
✓ ALL TESTS PASSED - JS→JS 7+ parameters bug is FIXED!
✓✓✓ ALL TEST SUITES PASSED ✓✓✓
```

## Architecture Details

### Bug A: DarwinPCS Variadic Calling Convention

**The fix**: In Lower::GenerateDirectCall, emit shadow-store instructions to write register arguments to stack, creating the contiguous frame that va_start expects.

### Bug B: JS→JS Overflow Argument Layout

**Status**: Requires investigation of offset computation mismatch between caller and callee.

See full documentation in thread summary for complete technical details.
