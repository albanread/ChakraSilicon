# ChakraSilicon# ChakraSilicon



**A port of Microsoft's ChakraCore JavaScript engine to Apple Silicon (ARM64 macOS)**> **⚠️ WORK IN PROGRESS - NOT READY FOR PRODUCTION USE ⚠️**

> 

ChakraSilicon is a working port of ChakraCore — the JavaScript engine that powered Microsoft Edge — to native Apple Silicon. Both the interpreter and the JIT compiler run natively on ARM64 macOS, with no Rosetta 2 translation required.> This project is under active development. Build scripts and functionality are being tested and validated.



## Status**ChakraCore builds for Apple Silicon (arm64) and Intel x86_64 with proper JIT support**



| Component | Status |This repository contains ChakraCore source code (previously a submodule, now fully integrated) along with build scripts to create multiple target variants for macOS and Linux.

|-----------|--------|

| Interpreter | ✅ Working |## Version

| SimpleJit | ✅ Working |

| FullJit | ✅ Working |**Current Version:** `1.0.0` (see [VERSION](VERSION))

| Exception handling | ✅ Working (setjmp/longjmp) |

| JIT assembly tracing | ✅ Working (Capstone) |## What is ChakraSilicon?

| Test suite (core JS) | ~86% passing |

| WASM | ❌ Not ported |ChakraSilicon is a streamlined build system for ChakraCore that produces multiple binary variants:

| Debugger protocol | ❌ Not ported |

| Intl (ICU) | ❌ Not linked |- **Interpreter-only builds** for testing and constrained environments

- **JIT-enabled builds** for production performance

## Quick Start- **Multi-architecture support** for both x86_64 and arm64 (Apple Silicon)



### PrerequisitesAll ChakraCore source code is included directly in this repository (no submodules required).



macOS on Apple Silicon (M1/M2/M3/M4), with:## Quick Start



```bash### Prerequisites

brew install cmake ninja icu4c

```- macOS or Linux

- CMake 3.10 or later

### Build- C/C++ compiler (clang/clang++ on macOS, gcc/g++ on Linux)

- Ninja (optional, but recommended for faster builds)

```bash

# Debug build (recommended for development)On macOS with Homebrew:

cd build/chjita64_debug```bash

cmake ../../ChakraCore \brew install cmake ninja

  -G Ninja \```

  -DCMAKE_BUILD_TYPE=Debug \

  -DSTATIC_LIBRARY=ON \### Build a Single Target

  -DCMAKE_CXX_COMPILER=clang++ \

  -DCMAKE_C_COMPILER=clang```bash

ninja -j10# Make scripts executable (first time only)

chmod +x scripts/*.sh

# Run

./bin/ch/ch -e "print('hello from Apple Silicon')"# Build JIT-enabled x86_64 variant

```./scripts/build_target.sh chjitx64



### Run the test suite# Build interpreter-only arm64 variant

./scripts/build_target.sh chinta64

```bash

python3 ChakraCore/test/runtests.py \# List all available targets

  -b /path/to/build/bin/ch/ch \./scripts/build_target.sh list

  -d --show-passes --variants interpreted```

```

### Build All Targets and Create Distribution

## What was ported

```bash

ChakraCore was built for Windows x64 and had a Linux ARM64 port (AAPCS64). Neither worked on macOS ARM64 because Apple's platform has its own ABI (DarwinPCS), its own exception handling, and its own memory protection model. This port fixes all of that.# Build all targets and create packaged distributions

./scripts/build_all_and_pack.sh all

### DarwinPCS calling convention```



Apple ARM64 differs from Linux ARM64 in a critical way: **variadic function arguments are always passed on the stack**, never in registers. ChakraCore's `JavascriptMethod` entry points are variadic (`RecyclableObject*, CallInfo, ...`), and the JIT compiler was only placing arguments in registers x0–x7.This will:

1. Build all four target variants

**Fix:** The JIT backend now emits `STR` instructions to spill all register arguments to the outgoing stack area before every `BLR` call. This mirrors what the `arm64_CallFunction` assembly trampoline already does. The `DECLARE_ARGS_VARARRAY` macro was rewritten for Apple ARM64 to extract arguments from the stack via `va_list`.2. Create per-target tarballs in `dist/packages/`

3. Create a complete release bundle in `dist/`

*Files: `lib/Backend/arm64/LowerMD.cpp`, `lib/Runtime/Language/Arguments.h`*4. Generate SHA256 checksums for all packages



### W^X memory protection## Available Build Targets



Apple Silicon enforces hardware W^X — memory pages cannot be simultaneously writable and executable. ChakraCore's JIT assumed `mprotect(PROT_READ|PROT_WRITE|PROT_EXEC)` would work. It doesn't on Apple Silicon; you get SIGBUS.| Target      | Architecture | JIT      | Description                        |

|-------------|--------------|----------|------------------------------------|

**Fix:** JIT code pages are allocated with `MAP_JIT` and toggled between writable and executable using `pthread_jit_write_protect_np()`. Write protection is managed per-thread around code emission.| `chintx64`  | x86_64       | Disabled | Interpreter-only, Intel 64-bit     |

| `chjitx64`  | x86_64       | Enabled  | JIT-enabled, Intel 64-bit          |

*Files: `lib/Common/Memory/CustomHeap.cpp`, `lib/Common/Memory/VirtualAllocWrapper.cpp`*| `chinta64`  | arm64        | Disabled | Interpreter-only, Apple Silicon    |

| `chjita64`  | arm64        | Enabled  | JIT-enabled, Apple Silicon         |

### Exception handling

## Repository Structure

ChakraCore on Linux ARM64 uses DWARF `.eh_frame` for exception unwinding. The DWARF emission was broken (wrong register numbers, missing CFA directives, incorrect FDE encoding). After fixing all of that, a deeper problem emerged: **Apple's `libunwind` cannot transition from dynamically-registered `.eh_frame` (JIT code) to the static compact `__unwind_info` format (C++ runtime)**. The unwind just stops at the JIT/C++ boundary.

```

**Fix:** Exception handling on Apple ARM64 uses `setjmp`/`longjmp` instead of DWARF unwinding. `OP_TryCatch`, `OP_TryFinally`, and `DoThrow` are implemented with `setjmp` at try-entry and `longjmp` at throw. This bypasses the unwinder entirely.ChakraSilicon/

├── ChakraCore/          # Full ChakraCore source (integrated, not a submodule)

*Files: `lib/Runtime/Language/JavascriptExceptionOperators.cpp`, `lib/Runtime/Base/ThreadContext.h`*├── scripts/             # Build and packaging scripts

│   ├── build_target.sh           # Build individual targets

### CALL_ENTRYPOINT macro│   └── build_all_and_pack.sh     # Build all targets and create packages

├── docs/                # Documentation and implementation notes

When C++ code calls a `JavascriptMethod` entry point, it goes through the `CALL_ENTRYPOINT` macro. On DarwinPCS, calling a variadic function pointer means the compiler puts args on the stack, but the JIT-compiled callee expects them in registers.├── examples/            # JavaScript test files and examples

├── legacy/              # Old build scripts (archived)

**Fix:** The macro casts the variadic function pointer to a non-variadic type with 16 `uintptr_t` parameters. The first 8 go into registers (what the JIT expects), and parameters 9–16 duplicate them onto the stack (what `va_list` reads).├── dist/                # Build outputs (created during build)

│   ├── chintx64/        # Interpreter x86_64 binaries

*File: `lib/Runtime/Language/Arguments.h`*│   ├── chjitx64/        # JIT x86_64 binaries

│   ├── chinta64/        # Interpreter arm64 binaries

### Prolog/Epilog code generation│   ├── chjita64/        # JIT arm64 binaries

│   └── packages/        # Distribution tarballs

ARM64 prolog encoding had issues with `STP`/`LDP` pair instructions that didn't match the DWARF CFI directives. These were split into individual `STR`/`LDR` instructions with matching CFI for each.├── build/               # Build directories (created during build)

├── VERSION              # Current version number

*Files: `lib/Backend/arm64/LowerMD.cpp`, `lib/Backend/PrologEncoder.cpp`, `lib/Backend/arm64/PrologEncoderMD.cpp`*└── README.md           # This file

```

## Architecture

## Usage Examples

```

┌─────────────────────────────────────────────────┐### Building Specific Targets

│                  JavaScript                      │

├─────────────────────────────────────────────────┤```bash

│              ChakraCore Frontend                 │# Build only the JIT-enabled x86_64 variant

│         (Parser → Bytecode → IR)                 │./scripts/build_target.sh chjitx64

├─────────────────────────────────────────────────┤

│            ARM64 JIT Backend                     │# Build multiple specific targets

│  ┌──────────────┐  ┌─────────────────────────┐  │./scripts/build_all_and_pack.sh chjitx64 chinta64

│  │ LowerMD.cpp  │  │ Encoder.cpp             │  │```

│  │ (IR → ARM64) │  │ (ARM64 → machine code)  │  │

│  └──────────────┘  └─────────────────────────┘  │### Running ChakraCore

├─────────────────────────────────────────────────┤

│           Apple Silicon Platform                 │After building, binaries are in `dist/<target>/`:

│  ┌────────────┐ ┌───────────┐ ┌──────────────┐  │

│  │ DarwinPCS  │ │ MAP_JIT   │ │ setjmp/      │  │```bash

│  │ va_list    │ │ W^X       │ │ longjmp EH   │  │# Run the JIT-enabled x86_64 build

│  └────────────┘ └───────────┘ └──────────────┘  │./dist/chjitx64/ch examples/simple_test.js

└─────────────────────────────────────────────────┘

```# Run the arm64 interpreter build

./dist/chinta64/ch examples/jit_demo.js

## Repository structure```



```### Cleaning Build Artifacts

ChakraSilicon/

├── ChakraCore/           # Full ChakraCore source (no submodules)```bash

│   ├── lib/Backend/      # JIT compiler backend (ARM64 port changes here)# Clean all build artifacts and distribution files

│   ├── lib/Runtime/      # Runtime library (calling convention, EH changes here)./scripts/build_all_and_pack.sh clean

│   └── lib/Common/       # Memory management (W^X changes here)```

├── build/                # Build output directories

├── docs/                 # Port documentation## Build Output

│   ├── APPLE_SILICON_PORT.md

│   └── stackframes.mdEach target produces:

├── examination/          # Debug scripts and analysis notes

├── examples/             # Test JavaScript files- `ch` - ChakraCore shell (command-line JavaScript engine)

└── scripts/              # Build scripts- `libChakraCore.dylib` (macOS) or `libChakraCore.so` (Linux) - Shared library

```- `libChakraCoreStatic.a` - Static library

- `build_info.txt` - Build metadata and configuration

## Known limitations

Distribution packages:

- **WASM**: WebAssembly is not ported. The WASM JIT would need the same DarwinPCS and W^X fixes.

- **Debugger**: The VS Code debugger protocol is not ported.- `dist/packages/<target>-<version>.tar.gz` - Individual target tarballs

- **Intl**: The ICU internationalization library is not linked. `Intl` objects will throw.- `dist/ChakraSilicon-<version>-<platform>.tar.gz` - Complete release bundle

- **JIT codegen**: Some JIT optimization paths may still have ARM64-specific issues. Around 14% of tests fail, mostly in advanced optimizer and ES6 edge cases.- `*.sha256` - SHA256 checksums for verification

- **No iOS**: This targets macOS only. iOS would need additional entitlements for JIT.

## Platform Support

## Building other variants

### macOS (Primary)

```bash

# x86_64 JIT (for Intel Macs or Rosetta)- **Apple Silicon (M1/M2/M3)**: Full support for both interpreter and JIT modes

./scripts/build_target.sh chjitx64- **Intel x86_64**: Full support for both interpreter and JIT modes

- Universal binaries can be created by building both architectures

# ARM64 interpreter only (no JIT)

./scripts/build_target.sh chinta64### Linux (Experimental)



# All variants- **x86_64**: Supported

./scripts/build_all_and_pack.sh all- **arm64**: Supported on ARM64 Linux systems

```

## Important Notes

## License

### Submodules

ChakraCore is licensed under the MIT License. See [ChakraCore/LICENSE.txt](ChakraCore/LICENSE.txt).

**This repository no longer uses git submodules.** ChakraCore and capstone source code are fully integrated into this repository. No `git submodule` commands are needed.

## Credits

### JIT on Apple Silicon

- **ChakraCore**: Microsoft and [ChakraCore contributors](https://github.com/nicolo-ribaudo/ChakraCore)

- **Capstone**: Disassembly framework used for JIT assembly tracingThe JIT implementation for Apple Silicon (`chjita64`) includes platform-specific modifications for proper operation on ARM64 macOS. See the documentation in `docs/` for implementation details.

- **Apple Silicon port**: Alban Read

### Build System

- Builds use CMake with Ninja (preferred) or Unix Makefiles
- All builds are Release configuration by default
- Parallel builds are enabled automatically based on CPU core count

## Documentation

Additional documentation is available in the `docs/` directory:

- [SUBMODULE_CONVERSION.md](docs/SUBMODULE_CONVERSION.md) - Details on the submodule-to-flat-repo conversion
- [PHASE1_IMPLEMENTATION_SUMMARY.md](docs/PHASE1_IMPLEMENTATION_SUMMARY.md) - Implementation notes
- [JIT_ASSEMBLY_TRACING_IMPLEMENTATION.md](docs/JIT_ASSEMBLY_TRACING_IMPLEMENTATION.md) - JIT tracing details
- [FINAL_BUILD_REPORT.md](docs/FINAL_BUILD_REPORT.md) - Build system report

## Examples

JavaScript example files are in the `examples/` directory:

- `simple_test.js` - Basic JavaScript execution test
- `jit_demo.js` - JIT compilation demonstration
- `jit_performance_test.js` - Performance comparison
- And more...

## Contributing

This is a specialized build of ChakraCore. For ChakraCore itself, see the [official ChakraCore repository](https://github.com/Microsoft/ChakraCore).

## License

ChakraCore is licensed under the MIT License. See [ChakraCore/LICENSE.txt](ChakraCore/LICENSE.txt) for details.

## Credits

- **ChakraCore**: Microsoft ChakraCore Team
- **Capstone**: The Capstone disassembly framework
- **ChakraSilicon**: Build system and Apple Silicon adaptations

## Support

For issues specific to this build system, please open an issue in this repository.

For ChakraCore issues, refer to the [official ChakraCore repository](https://github.com/Microsoft/ChakraCore).

---

**Built with ❤️ for Apple Silicon and x86_64**