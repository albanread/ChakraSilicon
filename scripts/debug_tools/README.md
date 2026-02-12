# Debug Tools

This directory contains scripts and LLDB command files used to debug the Apple Silicon port of ChakraCore. These tools were essential in diagnosing issues with JIT compilation, stack frame layout, and exception handling.

## LLDB Scripts (`.lldb`)

These scripts automate `lldb` sessions to capture specific state or trace execution flow. They are typically used with `command source` inside LLDB.

*   **`debug_x64_thunk.lldb`**: Analyzes the working x64 interpreter thunk to understand how it handles stack frames and parameters. Used as a reference for fixing the ARM64 thunk.
*   **`debug_thunk.lldb`**: Debugs the ARM64 interpreter thunk generation and execution.
*   **`debug_hang.lldb`**: Scripts to diagnose infinite loops or hangs in JIT code, specifically related to the JIT-to-Interpreter transition.
*   **`debug_trycatch.lldb` / `debug_trycatch2.lldb`**: Traces exception handling flow, verifying `setjmp`/`longjmp` behavior and stack unwinding.
*   **`research_x64.lldb`**: Exploratory script for gathering x64 runtime details.

### Usage

Start LLDB with the target binary:

```bash
lldb ../../dist/chjita64/ch
```

Then, inside LLDB, load a script:

```lldb
(lldb) command source debug_hang.lldb
```

## Shell Scripts (`.sh`)

Wrapper scripts to launch debugging sessions or reproduce specific crash scenarios.

*   **`debug_interactive.sh`**: Helper to launch an interactive debugging session with environment variables set up correctly.
*   **`hang_and_attach.sh`**: Runs a test case known to hang (like `test_jit_to_int.js`) in the background and attaches LLDB to it automatically.
*   **`debug_fulljit_crash.sh`**: automate debugging of FullJIT crashes.
*   **`debug_fulljit_exception.sh`**: automate debugging of FullJIT exception handling failures.

## Workflow

1.  Identify a failing test case (e.g., in `tests/repro_cases/`).
2.  Use a shell script to run it under a debugger or attach to it.
3.  Use an LLDB script to automate the setup of breakpoints and state inspection.