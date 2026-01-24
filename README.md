# ChakraSilicon

**ChakraCore builds for Apple Silicon (arm64) and Intel x86_64 with proper JIT support**

This repository contains ChakraCore source code (previously a submodule, now fully integrated) along with build scripts to create multiple target variants for macOS and Linux.

## Version

**Current Version:** `1.0.0` (see [VERSION](VERSION))

## What is ChakraSilicon?

ChakraSilicon is a streamlined build system for ChakraCore that produces multiple binary variants:

- **Interpreter-only builds** for testing and constrained environments
- **JIT-enabled builds** for production performance
- **Multi-architecture support** for both x86_64 and arm64 (Apple Silicon)

All ChakraCore source code is included directly in this repository (no submodules required).

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

## Available Build Targets

| Target      | Architecture | JIT      | Description                        |
|-------------|--------------|----------|------------------------------------|
| `chintx64`  | x86_64       | Disabled | Interpreter-only, Intel 64-bit     |
| `chjitx64`  | x86_64       | Enabled  | JIT-enabled, Intel 64-bit          |
| `chinta64`  | arm64        | Disabled | Interpreter-only, Apple Silicon    |
| `chjita64`  | arm64        | Enabled  | JIT-enabled, Apple Silicon         |

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
└── README.md           # This file
```

## Usage Examples

### Building Specific Targets

```bash
# Build only the JIT-enabled x86_64 variant
./scripts/build_target.sh chjitx64

# Build multiple specific targets
./scripts/build_all_and_pack.sh chjitx64 chinta64
```

### Running ChakraCore

After building, binaries are in `dist/<target>/`:

```bash
# Run the JIT-enabled x86_64 build
./dist/chjitx64/ch examples/simple_test.js

# Run the arm64 interpreter build
./dist/chinta64/ch examples/jit_demo.js
```

### Cleaning Build Artifacts

```bash
# Clean all build artifacts and distribution files
./scripts/build_all_and_pack.sh clean
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

## Platform Support

### macOS (Primary)

- **Apple Silicon (M1/M2/M3)**: Full support for both interpreter and JIT modes
- **Intel x86_64**: Full support for both interpreter and JIT modes
- Universal binaries can be created by building both architectures

### Linux (Experimental)

- **x86_64**: Supported
- **arm64**: Supported on ARM64 Linux systems

## Important Notes

### Submodules

**This repository no longer uses git submodules.** ChakraCore and capstone source code are fully integrated into this repository. No `git submodule` commands are needed.

### JIT on Apple Silicon

The JIT implementation for Apple Silicon (`chjita64`) includes platform-specific modifications for proper operation on ARM64 macOS. See the documentation in `docs/` for implementation details.

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