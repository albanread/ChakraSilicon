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
| Test suite (core JS) | ~91% passing (1383/1526 excl. WASM) |
| WASM | ❌ Not ported |
| Debugger protocol | ❌ Not ported |
| Intl (ICU) | ✅ Working (Homebrew icu4c) |
| Unicode normalization | ✅ Working (NFC/NFD/Hangul) |

## Test Results

> **Last run:** 2026-02-13 — Binary: `dist/chjita64/ch` (JIT-enabled ARM64) — Timeout: 5s/test

### Summary

| Metric | Count |
|--------|-------|
| **Total tests** | 2748 |
| **Ran** | 1649 |
| **Passed** | 1383 |
| **Failed** | 266 |
| **Skipped** | 1099 |
| **Overall pass rate** | **83%** (1383/1649) |
| **Core JS pass rate** (excl. WasmSpec) | **91%** (1383/1526) |
| Folders tested | 62 of 70 |
| Time | 114s |

### Per-Folder Breakdown

| Folder | Pass | Fail | Skip | Total | Rate |
|--------|-----:|-----:|-----:|------:|-----:|
| 262 | 1 | 0 | 0 | 1 | 100% |
| Array | 85 | 1 | 45 | 131 | 98% |
| Basics | 46 | 2 | 4 | 52 | 95% |
| BigInt | 11 | 0 | 0 | 11 | 100% |
| Boolean | 2 | 0 | 1 | 3 | 100% |
| Closures | 15 | 1 | 14 | 30 | 93% |
| ConfigParsing | 0 | 5 | 0 | 5 | 0% |
| ControlFlow | 16 | 1 | 0 | 17 | 94% |
| Conversions | 5 | 0 | 0 | 5 | 100% |
| Date | 8 | 1 | 14 | 23 | 88% |
| Debugger | 16 | 3 | 1 | 20 | 84% |
| DebuggerCommon | 132 | 18 | 60 | 210 | 88% |
| Error | 9 | 12 | 8 | 29 | 42% |
| FixedFields | 9 | 0 | 9 | 18 | 100% |
| Function | 54 | 1 | 27 | 82 | 98% |
| Generated | 110 | 0 | 0 | 110 | 100% |
| GlobalFunctions | 12 | 1 | 2 | 15 | 92% |
| InlineCaches | 23 | 1 | 0 | 24 | 95% |
| JSON | 15 | 1 | 3 | 19 | 93% |
| JsBuiltIns | 3 | 0 | 0 | 3 | 100% |
| LetConst | 36 | 3 | 24 | 63 | 92% |
| Lib | 10 | 1 | 0 | 11 | 90% |
| Math | 9 | 0 | 0 | 9 | 100% |
| Number | 9 | 1 | 1 | 11 | 90% |
| Object | 41 | 3 | 26 | 70 | 93% |
| Optimizer | 153 | 1 | 106 | 260 | 99% |
| PRE | 1 | 0 | 1 | 2 | 100% |
| Prototypes | 8 | 0 | 1 | 9 | 100% |
| RWC | 1 | 0 | 0 | 1 | 100% |
| Regex | 33 | 4 | 4 | 41 | 89% |
| Scanner | 2 | 1 | 0 | 3 | 66% |
| StackTrace | 2 | 12 | 4 | 18 | 14% |
| Strings | 36 | 4 | 4 | 44 | 90% |
| TaggedIntegers | 14 | 0 | 30 | 44 | 100% |
| TryCatch | 1 | 0 | 0 | 1 | 100% |
| UnifiedRegex | 20 | 2 | 5 | 27 | 90% |
| UnitTestFramework | 0 | 1 | 0 | 1 | 0% |
| WasmSpec | 0 | 123 | 108 | 231 | 0% |
| WasmSpec.MultiValue | 2 | 0 | 0 | 2 | 100% |
| bailout | 17 | 1 | 20 | 38 | 94% |
| es6 | 155 | 17 | 72 | 244 | 90% |
| es6module | 16 | 1 | 14 | 31 | 94% |
| es7 | 19 | 1 | 4 | 24 | 95% |
| fieldopts | 35 | 8 | 79 | 122 | 81% |
| inlining | 20 | 4 | 27 | 51 | 83% |
| loop | 4 | 0 | 6 | 10 | 100% |
| stackfunc | 4 | 0 | 85 | 89 | 100% |
| strict | 64 | 1 | 42 | 107 | 98% |
| switchStatement | 14 | 0 | 12 | 26 | 100% |
| typedarray | 34 | 12 | 17 | 63 | 73% |
| utf8 | 3 | 2 | 4 | 9 | 60% |
| wasm | 37 | 15 | 14 | 66 | 71% |
| wasm.simd | 11 | 0 | 1 | 12 | 100% |

Folders with all tests skipped (feature-gated): ASMJSParser, AsmJSFloat, DynamicCode, FlowGraph, PerfHint, RegAlloc, TaggedFloats, TTBasic, TTExecuteBasic.

### Hanging Folders (8)

The following 8 folders cause the test runner to hang and were excluded:

`AsmJs`, `Bugs`, `EH`, `es5`, `es6GeneratorJit`, `Intl`, `Miscellaneous`, `Operators`

### Notable Failure Areas

- **WasmSpec** (123 failures) — Expected; WASM is not ported.
- **StackTrace** (12 failures) — Stack trace formatting differences on ARM64/macOS.
- **Error** (12 failures) — Error message and source-info baseline mismatches.
- **DebuggerCommon** (18 failures) — Debugger protocol is not ported.
- **es6** (17 failures) — Mostly `__proto__` initializer and `let`/`const` global shadow edge cases.
- **typedarray** (12 failures) — TypedArray edge cases.
- **wasm** (15 failures) — WASM JS API mismatches.
- **fieldopts** (8 failures) — Field optimization edge cases.

### 100% Pass-Rate Folders

The following 18 folders passed every test that was run:

`262`, `BigInt`, `Boolean`, `Conversions`, `FixedFields`, `Generated`, `JsBuiltIns`, `Math`, `PRE`, `Prototypes`, `RWC`, `TaggedIntegers`, `TryCatch`, `WasmSpec.MultiValue`, `loop`, `stackfunc`, `switchStatement`, `wasm.simd`

Near-perfect (≥95%): Array (98%), Basics (95%), Function (98%), Optimizer (99%), strict (98%), InlineCaches (95%), es7 (95%).

## JavaScript Language Support

ChakraSilicon has **complete ES2019 (ES10) support**, plus cherry-picked features from ES2020–ES2022. The main gaps are optional chaining (`?.`), BigInt literals, logical assignment operators, private class fields, and `WeakRef`/`FinalizationRegistry`.

### ES5 – ES2019: Full Support ✅

| Standard | Key Features |
|----------|-------------|
| **ES5** (2009) | `JSON`, `Array.isArray`, `Object.keys`, `Function.bind`, strict mode |
| **ES2015** (ES6) | `let`/`const`, arrow functions, classes, template literals, destructuring, default/rest params, spread, `Symbol`, `Promise`, `Map`/`Set`/`WeakMap`/`WeakSet`, `Proxy`/`Reflect`, `for...of`, generators, iterators, `Array.from`, `Object.assign`, ES6 modules |
| **ES2016** (ES7) | `Array.prototype.includes`, exponentiation operator (`**`) |
| **ES2017** (ES8) | `async`/`await`, `Object.values`/`Object.entries`, `String.padStart`/`padEnd`, `Object.getOwnPropertyDescriptors` |
| **ES2018** (ES9) | Object rest/spread, RegExp dotAll (`s` flag), async iteration (`async function*`), `Promise.prototype.finally` |
| **ES2019** (ES10) | `Array.flat`/`flatMap`, `String.trimStart`/`trimEnd`, `Object.fromEntries`, `Symbol.description`, optional catch binding |

### ES2020 – ES2022: Partial Support ⚠️

| Feature | Standard | Status |
|---------|----------|--------|
| `globalThis` | ES2020 | ✅ |
| `Promise.allSettled` | ES2020 | ✅ |
| Nullish coalescing (`??`) | ES2020 | ✅ |
| `Promise.any` | ES2021 | ✅ |
| `AggregateError` | ES2021 | ✅ |
| Numeric separators (`1_000_000`) | ES2021 | ✅ |
| `Array.at` | ES2022 | ✅ |
| `Object.hasOwn` | ES2022 | ✅ |
| `Error.cause` | ES2022 | ✅ |
| `Array.findLast` / `findLastIndex` | ES2022 | ✅ |
| Optional chaining (`?.`) | ES2020 | ❌ |
| BigInt literals (`1n`) | ES2020 | ❌ |
| `String.matchAll` | ES2020 | ❌ |
| `String.replaceAll` | ES2021 | ❌ |
| `WeakRef` / `FinalizationRegistry` | ES2021 | ❌ |
| Logical assignment (&#124;&#124;=, &&=, ??=) | ES2021 | ❌ |
| Private class fields (`#x`) | ES2022 | ❌ |
| Static class blocks | ES2022 | ❌ |

### ES2023+: Not Supported ❌

`Array.toSorted`, `Array.toReversed`, `Array.toSpliced`, `Array.with` — all absent.

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
- **JIT codegen**: Some JIT optimization paths may still have ARM64-specific issues. ~9% of core JS tests fail (excluding WASM), mostly in error formatting, debugger, ES6 edge cases, and stack traces.
- **Hanging tests**: 8 test folders (AsmJs, Bugs, EH, es5, es6GeneratorJit, Intl, Miscellaneous, Operators) cause the test runner to hang and must be skipped.
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
