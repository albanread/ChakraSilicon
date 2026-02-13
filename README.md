# ChakraSilicon

## Why?

When Chakra was released, I really enjoyed reading the source code, it was nicely designed, and
comprehensible. Unlike some other JavaScript engines. 

I want to be able to run it on my own computer, so here we are.


> **⚠️ NOT MAINTAINED ⚠️**
> You are welcome to fork this and maintain it for yourself.
> This project does not accept pull requests.

**A port of Microsoft's ChakraCore JavaScript engine to Apple Silicon (ARM64 macOS)**

ChakraSilicon is a working port of ChakraCore — the JavaScript engine that powered Microsoft Edge — to native Apple Silicon. Both the interpreter and the JIT compiler run natively on ARM64 macOS, with no Rosetta 2 translation required.

**Current Version:** `1.0.0` (see [VERSION](VERSION))

## Status

| Component | Status |
|-----------|--------|
| Interpreter | ✅ Working |
| SimpleJit | ✅ Working |
| FullJit | ✅ Working |
| Exception handling | ✅ Working (setjmp/longjmp) |
| JIT assembly tracing | ✅ Working (Capstone) |
| Test suite (core JS) | ~86% passing |
| WASM | ❌ Not ported |
| Debugger protocol | ❌ Not ported |
| Intl (ICU) | ✅ Working (Homebrew icu4c) |
| Unicode normalization | ✅ Working (NFC/NFD/Hangul) |

## Quick Start

### Prerequisites

- macOS or Linux
- CMake 3.10 or later
- C/C++ compiler (clang/clang++ on macOS, gcc/g++ on Linux)
- Ninja (optional, but recommended for faster builds)
- ICU (`icu4c`) — required for `Intl` API, `String.prototype.normalize`, and full Unicode case mapping (e.g. `ß` → `SS`)

On macOS with Homebrew:

```bash
brew install cmake ninja icu4c
```

The build script auto-detects Homebrew ICU (including versioned formulae like `icu4c@77`). If ICU is not found, the build will still succeed but without `Intl` support or Unicode normalization.

### Build a Single Target

```bash
# Make scripts executable (first time only)
chmod +x scripts/*.sh

# Build JIT-enabled x86_64 variant
./scripts/build_target.sh chjitx64

# Build interpreter-only arm64 variant
./scripts/build_target.sh chinta64

# List all available targets
./scripts/build_target.sh list
```

### Build All Targets and Create Distribution

```bash
# Build all targets and create packaged distributions
./scripts/build_all_and_pack.sh all
```

This will:
1. Build all four target variants
2. Create per-target tarballs in `dist/packages/`
3. Create a complete release bundle in `dist/`
4. Generate SHA256 checksums for all packages

### Run

```bash
./dist/chjitx64/ch examples/simple_test.js
```

## Available Build Targets

| Target      | Architecture | JIT      | Description                        |
|-------------|--------------|----------|------------------------------------|
| `chintx64`  | x86_64       | Disabled | Interpreter-only, Intel 64-bit     |
| `chjitx64`  | x86_64       | Enabled  | JIT-enabled, Intel 64-bit          |
| `chinta64`  | arm64        | Disabled | Interpreter-only, Apple Silicon    |
| `chjita64`  | arm64        | Enabled  | JIT-enabled, Apple Silicon         |

## Execution Pipeline

ChakraCore uses a **tiered execution model** — every JavaScript function progresses through increasingly optimized execution modes based on how often it is called. The tiers are controlled by a state machine (`FunctionExecutionStateMachine`) that tracks call counts and decides when to promote a function to the next tier.

### Interpreter

When a function is first called, it runs in the **bytecode interpreter**. The parser converts JavaScript source into an AST, the `ByteCodeGenerator` walks the AST and emits a compact bytecode (register-based, not stack-based), and the interpreter loop (`InterpreterStackFrame`) dispatches each opcode via a large `switch` statement — each `case` reads an `OpLayout`, executes the operation, and advances the instruction pointer.

The interpreter has three sub-modes:

- **Interpreter** — no profiling, minimal overhead. Used in `--no-native` mode or when JIT is disabled.
- **AutoProfilingInterpreter** — starts with lightweight profiling, switches to full profiling inside hot loops based on iteration count, and switches back on loop exit. This avoids the cost of profiling cold code while still collecting data for hot paths.
- **ProfilingInterpreter** — full profiling on every operation. Records value types, call targets, branch directions, and array access patterns. This profiling data is what the JIT uses to specialize code.

Each function is given an initial budget of interpreted iterations (configurable, defaults around 12 auto-profiling + 4 profiling iterations) before the JIT is considered.

### SimpleJit

After the interpreter budget is exhausted (default ~21 iterations), the function is queued for **SimpleJit** compilation. SimpleJit is a fast, lightweight JIT that compiles bytecode to native machine code with minimal optimization:

- No global optimizer (`GlobOpt` is skipped)
- No type specialization, no copy propagation, no dead store elimination
- No loop invariant hoisting or bounds check elimination
- Does include fast paths for common operations (property access, arithmetic)
- Generates profiling information that feeds into FullJit decisions

SimpleJit compiles quickly and produces code that runs 2–5× faster than the interpreter, but the generated machine code is not heavily optimized. Its main purpose is to bridge the gap while FullJit compilation happens on a background thread, and to collect the remaining profile data the FullJit optimizer needs.

### FullJit

Once the full JIT threshold is reached (the sum of all tier budgets, adjusted by heuristics based on loop density and function size), the function is queued for **FullJit** compilation on a background thread (`NativeCodeGenerator`). This is the heavy-duty optimizing compiler:

1. **IRBuilder** — converts bytecode + profiling data into ChakraCore's intermediate representation (IR), a graph of `IR::Instr` nodes in SSA-like form.

2. **GlobOpt** (Global Optimizer) — the core optimization pass. Performs:
   - **Type specialization** — uses profiling data to specialize operations to `int32`, `float64`, or specific object types, inserting bailout checks for type guards.
   - **Copy propagation** — eliminates redundant loads by tracking which registers hold the same value.
   - **Dead store elimination** — removes writes that are never read.
   - **Constant folding** — evaluates expressions with known constant operands at compile time.
   - **Loop invariant code motion** — hoists computations that don't change across loop iterations out of the loop.
   - **Array bounds check hoisting/elimination** — removes or hoists redundant array bounds checks by proving index ranges.
   - **Inlining** — inlines small callees and built-in functions directly into the caller.
   - **Path-dependent value analysis** — tracks value ranges along different branch paths.

3. **Lowerer** — converts high-level IR into machine-specific IR. The `LowererMD` (machine-dependent lowerer) handles ARM64-specific instruction selection — mapping IR opcodes to ARM64 instructions, register allocation constraints, and calling convention requirements.

4. **Register Allocator** — linear scan register allocator assigns physical ARM64 registers (`x0`–`x28`, `d0`–`d31`) to IR operands, inserting spills/fills as needed.

5. **Encoder** — final pass that emits actual ARM64 machine code bytes into an executable buffer, resolves branch targets, and emits relocation entries.

6. **Prolog/Epilog** — the `PrologEncoder` emits function entry/exit code (frame setup, callee-saved register saves/restores) with matching DWARF CFI directives.

If a type guard or bounds check fails at runtime, the JIT code **bails out** — it transfers control back to the interpreter at the corresponding bytecode offset, preserving all live values. The function may later be re-JIT'd with updated profiling data.

### JIT Assembly Tracing (Capstone)

ChakraSilicon integrates the [Capstone](https://www.capstone-engine.org/) disassembly engine to provide **JIT assembly tracing** — the ability to inspect the native ARM64 (or x86-64) machine code that the JIT compiler produces. This is invaluable for debugging the JIT backend and understanding what code the optimizer actually generates.

When tracing is enabled, the `JitAsmTracer` class:

1. Captures the raw machine code bytes emitted by the JIT encoder for each compiled function.
2. Calls `cs_disasm()` to disassemble the bytes into human-readable assembly.
3. Performs **control flow analysis** — identifies branches, calls, and returns to map out basic blocks.
4. Performs **register usage analysis** — tracks reads and writes to each register.
5. Prints a formatted trace with address, raw bytes, and disassembled instructions, annotated with control flow markers (`[C]` call, `[B]` branch, `[R]` return).
6. Reports performance metrics: instruction mix (math/memory/control), branch density, and register pressure.

Capstone is built as a static library from source (`deps/capstone/`), configured to include only the architecture matching the build target (ARM64 for Apple Silicon, x86-64 for Intel). It is linked into the `ch` shell and the ChakraCore library via the `ChakraCapstone` CMake interface library.

## What was ported or perhaps 'hacked just enough to work'

ChakraCore was built for Windows x64 and had a Linux ARM64 port (AAPCS64). 
Apple's platform has its own ABI (DarwinPCS), its own exception handling, and its own memory protection model. That did not work for ARN64, This version fixes all of that.

### DarwinPCS calling convention

Apple ARM64 differs from Linux ARM64 in a critical way: **variadic function arguments are always passed on the stack**, never in registers. ChakraCore's `JavascriptMethod` entry points are variadic (`RecyclableObject*, CallInfo, ...`), and the JIT compiler was only placing arguments in registers x0–x7.

**Fix:** The JIT backend now emits `STR` instructions to spill all register arguments to the outgoing stack area before every `BLR` call. This mirrors what the `arm64_CallFunction` assembly trampoline already does. The `DECLARE_ARGS_VARARRAY` macro was rewritten for Apple ARM64 to extract arguments from the stack via `va_list`.

*Files: `lib/Backend/arm64/LowerMD.cpp`, `lib/Runtime/Language/Arguments.h`*

### W^X memory protection

Apple Silicon enforces hardware W^X — memory pages cannot be simultaneously writable and executable. ChakraCore's JIT assumed `mprotect(PROT_READ|PROT_WRITE|PROT_EXEC)` would work. It doesn't on Apple Silicon; you get SIGBUS.

**Fix:** JIT code pages are allocated with `MAP_JIT` and toggled between writable and executable using `pthread_jit_write_protect_np()`. Write protection is managed per-thread around code emission.

*Files: `lib/Common/Memory/CustomHeap.cpp`, `lib/Common/Memory/VirtualAllocWrapper.cpp`*

### Exception handling

ChakraCore on Linux ARM64 uses DWARF `.eh_frame` for exception unwinding. The DWARF emission was broken (wrong register numbers, missing CFA directives, incorrect FDE encoding). After fixing all of that, a deeper problem emerged: **Apple's `libunwind` cannot transition from dynamically-registered `.eh_frame` (JIT code) to the static compact `__unwind_info` format (C++ runtime)**. The unwind just stops at the JIT/C++ boundary.

**Fix:** Exception handling on Apple ARM64 uses `setjmp`/`longjmp` instead of DWARF unwinding. `OP_TryCatch`, `OP_TryFinally`, and `DoThrow` are implemented with `setjmp` at try-entry and `longjmp` at throw. This bypasses the unwinder entirely.

*Files: `lib/Runtime/Language/JavascriptExceptionOperators.cpp`, `lib/Runtime/Base/ThreadContext.h`*

### CALL_ENTRYPOINT macro

When C++ code calls a `JavascriptMethod` entry point, it goes through the `CALL_ENTRYPOINT` macro. On DarwinPCS, calling a variadic function pointer means the compiler puts args on the stack, but the JIT-compiled callee expects them in registers.

**Fix:** The macro casts the variadic function pointer to a non-variadic type with 16 `uintptr_t` parameters. The first 8 go into registers (what the JIT expects), and parameters 9–16 duplicate them onto the stack (what `va_list` reads).

*File: `lib/Runtime/Language/Arguments.h`*

### Prolog/Epilog code generation

ARM64 prolog encoding had issues with `STP`/`LDP` pair instructions that didn't match the DWARF CFI directives. These were split into individual `STR`/`LDR` instructions with matching CFI for each.

*Files: `lib/Backend/arm64/LowerMD.cpp`, `lib/Backend/PrologEncoder.cpp`, `lib/Backend/arm64/PrologEncoderMD.cpp`*

## Architecture

```
┌─────────────────────────────────────────────────┐
│                  JavaScript                      │
├─────────────────────────────────────────────────┤
│              ChakraCore Frontend                 │
│         (Parser → Bytecode → IR)                 │
├─────────────────────────────────────────────────┤
│            ARM64 JIT Backend                     │
│  ┌──────────────┐  ┌─────────────────────────┐  │
│  │ LowerMD.cpp  │  │ Encoder.cpp             │  │
│  │ (IR → ARM64) │  │ (ARM64 → machine code)  │  │
│  └──────────────┘  └─────────────────────────┘  │
├─────────────────────────────────────────────────┤
│           Apple Silicon Platform                 │
│  ┌────────────┐ ┌───────────┐ ┌──────────────┐  │
│  │ DarwinPCS  │ │ MAP_JIT   │ │ setjmp/      │  │
│  │ va_list    │ │ W^X       │ │ longjmp EH   │  │
│  └────────────┘ └───────────┘ └──────────────┘  │
└─────────────────────────────────────────────────┘
```

## Repository Structure

```
ChakraSilicon/
├── ChakraCore/          # Full ChakraCore source (integrated, not a submodule)
├── scripts/             # Build and packaging scripts
│   ├── build_target.sh           # Build individual targets
│   └── build_all_and_pack.sh     # Build all targets and create packages
├── docs/                # Documentation and implementation notes
├── examples/            # JavaScript test files and examples
├── legacy/              # Old build scripts (archived)
├── dist/                # Build outputs (created during build)
│   ├── chintx64/        # Interpreter x86_64 binaries
│   ├── chjitx64/        # JIT x86_64 binaries
│   ├── chinta64/        # Interpreter arm64 binaries
│   ├── chjita64/        # JIT arm64 binaries
│   └── packages/        # Distribution tarballs
├── build/               # Build directories (created during build)
├── VERSION              # Current version number
└── README.md            # This file
```

## Build Output

Each target produces:

- `ch` - ChakraCore shell (command-line JavaScript engine)
- `libChakraCore.dylib` (macOS) or `libChakraCore.so` (Linux) - Shared library
- `libChakraCoreStatic.a` - Static library
- `build_info.txt` - Build metadata and configuration

Distribution packages:

- `dist/packages/<target>-<version>.tar.gz` - Individual target tarballs
- `dist/ChakraSilicon-<version>-<platform>.tar.gz` - Complete release bundle
- `*.sha256` - SHA256 checksums for verification

## Known Limitations

- **WASM**: WebAssembly is not ported. The WASM JIT would need the same DarwinPCS and W^X fixes.
- **Debugger**: The VS Code debugger protocol is not ported.
- **Intl**: Requires Homebrew `icu4c` — the build script auto-detects it, but without ICU installed, `Intl` objects will throw and `String.prototype.normalize` will be a no-op.
- **JIT codegen**: Some JIT optimization paths may still have ARM64-specific issues. Around 14% of tests fail, mostly in advanced optimizer and ES6 edge cases.
- **No iOS**: This targets macOS only. iOS would need additional entitlements for JIT.

## Platform Support

### macOS (Primary)

- **Apple Silicon (M1/M2/M3/M4)**: Full support for both interpreter and JIT modes
- **Intel x86_64**: Full support for both interpreter and JIT modes
- Universal binaries can be created by building both architectures

### Linux (Experimental)

- **x86_64**: Supported
- **arm64**: Supported on ARM64 Linux systems

## Documentation

Additional documentation is available in the `docs/` directory:

- [APPLE_SILICON_PORT.md](docs/APPLE_SILICON_PORT.md) - Technical details on the Apple Silicon port
- [CAPSTONE_INTEGRATION.md](docs/CAPSTONE_INTEGRATION.md) - JIT assembly tracing implementation
- [SUBMODULE_CONVERSION.md](docs/SUBMODULE_CONVERSION.md) - Details on the submodule-to-flat-repo conversion
- [stackframes.md](docs/stackframes.md) - Stack frame documentation

## Examples

JavaScript example files are in the `examples/` directory:

- `simple_test.js` - Basic JavaScript execution test
- `jit_demo.js` - JIT compilation demonstration
- `jit_performance_test.js` - Performance comparison
- And more...

## Contributing

This is a specialized build of ChakraCore for Apple Silicon and x86_64 architectures. For issues specific to this build system, please open an issue in this repository.

For ChakraCore engine issues, refer to the [official ChakraCore repository](https://github.com/Microsoft/ChakraCore).

## License

ChakraCore is licensed under the MIT License. See [ChakraCore/LICENSE.txt](ChakraCore/LICENSE.txt) for details.

Capstone is licensed under the BSD 3-Clause License. See [ChakraCore/deps/capstone/LICENSES/LICENSE.TXT](ChakraCore/deps/capstone/LICENSES/LICENSE.TXT) for details.

## Credits

- **[ChakraCore](https://github.com/nicolo-ribaudo/ChakraCore)** — Microsoft and ChakraCore contributors. The JavaScript engine that powered Microsoft Edge, released under the MIT License. Copyright © Microsoft Corporation.
- **[Capstone](https://www.capstone-engine.org/)** — Multi-platform, multi-architecture disassembly framework by Nguyen Anh Quynh. Used for JIT assembly tracing and code analysis. Version 6.0.0, BSD 3-Clause License. Copyright © 2013, COSEINC.
- **[ICU](https://icu.unicode.org/)** — International Components for Unicode by the Unicode Consortium. Provides `Intl` API, Unicode normalization, and full case mapping. Linked dynamically from Homebrew `icu4c`.
- **Apple Silicon port** — Alban Read.

## Support

For issues specific to this build system, please open an issue in this repository.

For ChakraCore engine issues, refer to the [official ChakraCore repository](https://github.com/Microsoft/ChakraCore).

---

**Built with ❤️ for Apple Silicon and x86_64**
