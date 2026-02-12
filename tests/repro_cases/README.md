# Reproduction Test Cases

This directory contains minimal JavaScript test cases designed to reproduce specific bugs, crashes, or hangs encountered during the Apple Silicon porting process.

These tests are isolated from the main ChakraCore test suite to facilitate focused debugging and regression testing.

## Key Test Cases

### JIT-to-Interpreter Transition
*   **`test_jit_to_int.js`**: The primary test case for the JIT-to-Interpreter transition bug (the "infinite hang"). It forces a JIT-compiled function to call a built-in function (interpreted), which previously failed on ARM64 due to ABI mismatches.
*   **`early_hang_test.js`**: A simplified version of the above, used to catch the hang earlier in the execution flow.
*   **`trace_jit_calls.js`**: Designed to generate a clean trace of JIT calls for analysis.

### Exception Handling
*   **`test_exception.js`**: Basic test for `try/catch` functionality.
*   **`test_exception_no_new.js`**: Tests exception throwing without the `new` keyword.
*   **`test_exception_minimal_fulljit.js`**: Minimal test case to stress the FullJIT exception paths.
*   **`test_exception_comprehensive.js`**: A more complex suite verifying various exception scenarios (nested try/catch, finally blocks, etc.).

### Other
*   **`test_interpreter.js`**: Verifies the basic functionality of the interpreter before JIT is enabled.
*   **`minimal_trace.js`**: extremely small script to produce the smallest possible execution trace for debugging low-level instruction flow.
*   **`test_fulljit_crash_diag.js`**: A test case specifically crafted to reproduce a crash in the FullJIT compiler.

## Usage

You can run these tests using the ChakraCore shell (`ch`).

**Example (Interpreter only):**
```bash
./dist/chinta64/ch tests/repro_cases/test_interpreter.js
```

**Example (JIT enabled):**
```bash
./dist/chjita64/ch tests/repro_cases/test_jit_to_int.js
```

**Example (Tracing JIT assembly):**
```bash
./dist/chjita64/ch -TraceJitAsm tests/repro_cases/minimal_trace.js
```
