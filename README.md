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
| Intl (ICU) | ❌ Not linked |

## Quick Start

### Prerequisites

- macOS or Linux
- CMake 3.10 or later
- C/C++ compiler (clang/clang++ on macOS, gcc/g++ on Linux)
- Ninja (optional, but recommended for faster builds)

On macOS with Homebrew:

```bash
brew install cmake ninja
```

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
- **Intl**: The ICU internationalization library is not linked. `Intl` objects will throw.
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

## Credits

- **ChakraCore**: Microsoft and [ChakraCore contributors](https://github.com/nicolo-ribaudo/ChakraCore)
- **Capstone**: Disassembly framework used for JIT assembly tracing
- **Apple Silicon port**: Alban Read

## Support

For issues specific to this build system, please open an issue in this repository.

For ChakraCore engine issues, refer to the [official ChakraCore repository](https://github.com/Microsoft/ChakraCore).

---

**Built with ❤️ for Apple Silicon and x86_64**
