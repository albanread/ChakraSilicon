# Testing Guide: Apple ARM64 JIT Call Bugs

## Step-by-Step Test Execution

### Step 1: Build ChakraCore

Before running tests, you must build ChakraCore on Apple Silicon:

```bash
cd ChakraSilicon/ChakraCore

# Debug build (recommended for testing)
./build.sh --debug

# OR Release build (faster execution)
./build.sh --release
```

**Expected output**: 
- Build completes successfully
- Binary created at: `Build/Debug/ch` or `Build/Release/ch`

**Build time**: ~5-15 minutes depending on your machine

**If build fails**: 
- Ensure you have Xcode Command Line Tools: `xcode-select --install`
- Ensure you have CMake: `brew install cmake`
- Check for error messages about missing dependencies

### Step 2: Verify Build

```bash
# Check binary exists
ls -lh ChakraCore/Build/Debug/ch

# Test basic execution
ChakraCore/Build/Debug/ch -version

# Quick sanity check
echo "print('Hello from ChakraCore');" | ChakraCore/Build/Debug/ch -
```

**Expected output**: Version info and "Hello from ChakraCore"

### Step 3: Run Test Suite

```bash
cd ChakraSilicon/apple_arm64_jit_tests

# Run all tests (recommended)
./run_tests.sh all

# OR run specific test suites
./run_tests.sh varargs    # Bug A only
./run_tests.sh params     # Bug B only
./run_tests.sh quick      # Quick smoke test
```

**Test duration**: 
- Full suite (`all`): ~2-5 minutes
- Individual suite: ~1-2 minutes
- Quick test: ~30 seconds

### Step 4: Interpret Results

#### ✅ Success Output

```
=== Apple ARM64 JIT Call Bugs Test Suite ===

--- Bug A: CallDirect Varargs ---
Running with JIT DISABLED (baseline)...
✓ No-JIT run completed
  Passed: 32, Failed: 0

Running with JIT ENABLED (testing fix)...
✓ JIT-enabled run completed
  Passed: 32, Failed: 0

✓✓✓ ALL TESTS PASSED WITH JIT ENABLED ✓✓✓

--- Bug B: JS→JS 7+ Parameters ---
Running with JIT DISABLED (baseline)...
✓ No-JIT run completed
  Passed: 30, Failed: 0

Running with JIT ENABLED (testing fix)...
✓ JIT-enabled run completed
  Passed: 30, Failed: 0

✓✓✓ ALL TESTS PASSED WITH JIT ENABLED ✓✓✓

✓✓✓ ALL TEST SUITES PASSED ✓✓✓
The Apple ARM64 JIT call bugs appear to be FIXED!
```

#### ❌ Failure Output Examples

**Crash (segfault)**:
```
Running with JIT ENABLED (testing fix)...
✗ JIT-enabled run crashed or failed
Segmentation fault: 11
```

**Test failures**:
```
Running with JIT ENABLED (testing fix)...
✓ JIT-enabled run completed
  Passed: 28, Failed: 4

✗✗✗ SOME TESTS FAILED WITH JIT ENABLED ✗✗✗
```

### Step 5: Manual Testing (Optional)

Run individual test files manually for debugging:

```bash
cd ChakraSilicon

# Test with JIT enabled (default)
ChakraCore/Build/Debug/ch apple_arm64_jit_tests/test_calldirect_varargs.js

# Test with JIT disabled (baseline)
ChakraCore/Build/Debug/ch -maxInterpretCount:1 -maxSimpleJitRunCount:1 -bgjit- apple_arm64_jit_tests/test_calldirect_varargs.js

# Test with JIT but no background compilation
ChakraCore/Build/Debug/ch -bgjit- apple_arm64_jit_tests/test_js_7plus_params.js

# Verbose output with trace
ChakraCore/Build/Debug/ch -trace:* apple_arm64_jit_tests/test_calldirect_varargs.js
```

### Step 6: Debugging Failed Tests

If tests fail, use these debugging techniques:

#### A. Run with LLDB

```bash
lldb -- ChakraCore/Build/Debug/ch apple_arm64_jit_tests/test_calldirect_varargs.js
(lldb) run
# After crash:
(lldb) bt          # backtrace
(lldb) frame info  # current frame
(lldb) register read  # register state
```

#### B. Isolate Specific Test

Edit the test file to run only one test:

```javascript
// In test_calldirect_varargs.js, comment out all but one test:
// testTypedArrayIndexOf();
testTypedArrayFill();  // Test only this one
// testArrayIndexOf();
```

#### C. Add Debug Output

Add `print()` statements in test functions:

```javascript
function funcAccessEach(a, b, c, d, e, f, g, h, i, j) {
    print("Param 7 (g) = " + g + " (expected 7)");
    print("Param 8 (h) = " + h + " (expected 8)");
    print("Param 9 (i) = " + i + " (expected 9)");
    print("Param 10 (j) = " + j + " (expected 10)");
    // ...
}
```

#### D. Compare Disassembly

For deep debugging, you can inspect generated machine code:

```bash
# Enable JIT code dumping (requires source modification)
# Edit ChakraCore/lib/Common/ConfigFlagsList.h
# Set FLAGNUMBER(Dump, JITCode) to true by default

# Rebuild and run
cd ChakraCore && ./build.sh --debug
cd ../apple_arm64_jit_tests
../ChakraCore/Build/Debug/ch -dump:JITCode test_js_7plus_params.js > jit_output.txt
```

## Test Matrix Reference

| Test Mode | JIT Enabled | Background JIT | Comparison | Duration |
|-----------|-------------|----------------|------------|----------|
| `all` | Yes + No | Yes | Yes | ~5 min |
| `varargs` | Yes + No | Yes | Yes | ~2 min |
| `params` | Yes + No | Yes | Yes | ~2 min |
| `quick` | Yes only | Yes | No | ~30 sec |

## Environment Variables

```bash
# Custom binary path
export CHAKRA_BIN=/path/to/custom/ch
./run_tests.sh all

# Force clean rebuild
export REBUILD=1
cd ChakraCore && ./build.sh --clean && ./build.sh --debug
```

## Known Issues & Workarounds

### Issue: Test times out

**Symptom**: Test hangs indefinitely

**Workaround**: Kill and run with reduced iterations:
```bash
# Edit test file, reduce forceJIT iterations from 10000 to 1000
```

### Issue: Intermittent failures

**Symptom**: Tests pass sometimes, fail other times

**Cause**: Race condition in JIT compilation

**Workaround**: 
```bash
# Disable background JIT
ChakraCore/Build/Debug/ch -bgjit- test_file.js
```

### Issue: "Binary not found" error

**Symptom**: `run_tests.sh` can't find `ch` binary

**Solution**: Build ChakraCore first (see Step 1) or set `CHAKRA_BIN`

### Issue: Wrong architecture warning

**Symptom**: "WARNING: Not running on ARM64 architecture!"

**Explanation**: Tests are designed for Apple Silicon. Running on x86_64/Intel will produce this warning. Results may not be meaningful.

## Verification Checklist

After running tests, verify:

- [ ] ChakraCore built successfully on Apple Silicon (ARM64)
- [ ] Tests run without crashes
- [ ] All CallDirect varargs tests pass (32/32)
- [ ] All JS→JS 7+ parameter tests pass (30/30)
- [ ] JIT-enabled results match no-JIT baseline
- [ ] No segmentation faults or bus errors
- [ ] No TypeError exceptions from built-in methods
- [ ] Parameter values are correct (not corrupted or offset)

## Next Steps After Testing

### If All Tests Pass ✅

1. **Run full ChakraCore test suite**:
   ```bash
   cd ChakraCore
   python test/runtests.py
   ```

2. **Consider adding to CI**: Add these tests to automated build pipeline

3. **Document the fix**: Update release notes and bug tracking

### If Tests Fail ❌

1. **Capture failure details**:
   - Which specific tests fail?
   - Does it crash or return wrong values?
   - Is it consistent or intermittent?

2. **Review the fix implementation**:
   - Check `ChakraCore/lib/Backend/Lower.cpp` line ~12315
   - Verify shadow-store logic is active
   - Confirm `#if defined(_ARM64_) && defined(__APPLE__)` is true

3. **Instrument the code**:
   - Add debug prints in `GenerateDirectCall`
   - Use `arm64_DebugTrampoline` to dump state
   - Check generated assembly

4. **Report findings**: Document which tests fail and under what conditions

## Quick Reference Commands

```bash
# Build
cd ChakraSilicon/ChakraCore && ./build.sh --debug

# Run all tests
cd ../apple_arm64_jit_tests && ./run_tests.sh all

# Run specific test
./run_tests.sh varargs

# Manual test
../ChakraCore/Build/Debug/ch test_calldirect_varargs.js

# Debug test
lldb -- ../ChakraCore/Build/Debug/ch test_calldirect_varargs.js

# Check binary
ls -lh ../ChakraCore/Build/Debug/ch
```

## Support

For issues or questions:
1. Review the main `README.md` for technical background
2. Check the conversation thread that generated these tests
3. Examine the bug documentation: `Apple ARM64 JIT Call Bugs.md`
