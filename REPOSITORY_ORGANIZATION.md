# Repository Organization

## Overview

This document describes the complete organization structure of the ChakraSilicon repository, explaining the purpose of each directory and how to navigate the codebase.

## Repository Structure

```
ChakraSilicon/
├── ChakraCore/              # Full ChakraCore source code (integrated)
├── scripts/                 # Build and automation scripts
│   └── debug_tools/         # Debugging scripts and LLDB helpers
├── docs/                    # Documentation and technical notes
├── examples/                # JavaScript example files
├── tests/                   # Test suites and reproduction cases
│   └── repro_cases/         # Minimal reproduction scripts for bugs
├── dist/                    # Build outputs (generated)
├── build/                   # Build directories (generated)
├── examination/             # Analysis and investigation files
├── VERSION                  # Current version number (semantic versioning)
├── README.md                # Main documentation and quick start guide
├── CHANGELOG.md             # Version history and changes
├── REPOSITORY_ORGANIZATION.md  # This file
├── STATUS.md                # Current project status and roadmap
└── .gitignore               # Git ignore rules
```

## Directory Details

### ChakraCore/
**Purpose:** Complete ChakraCore JavaScript engine source code

**Contents:**
- Full ChakraCore implementation
- Capstone disassembly library (at `ChakraCore/deps/capstone/`)
- CMake build configuration
- Original ChakraCore documentation

**Note:** This is NOT a git submodule. The source code is fully integrated into this repository for simplified management and local modifications.

### scripts/
**Purpose:** Build automation and packaging scripts

**Files:**
- `build_target.sh` - Build individual target variants (chintx64, chjitx64, chinta64, chjita64)
- `build_all_and_pack.sh` - Orchestrator to build all targets and create distribution packages

**Subdirectories:**
- `debug_tools/` - Contains shell scripts and LLDB command files for debugging the engine (e.g., `debug_hang.lldb`, `debug_interactive.sh`).

**Usage:**
```bash
# Build a single target
./scripts/build_target.sh chjitx64

# Build all targets and package
./scripts/build_all_and_pack.sh all
```

### tests/
**Purpose:** Focused test cases and reproduction scripts

**Subdirectories:**
- `repro_cases/` - Contains minimal JavaScript files designed to reproduce specific bugs (e.g., JIT-to-Interpreter transitions, exception handling failures).

**Usage:**
```bash
# Run a specific reproduction case
./dist/chjita64/ch tests/repro_cases/test_jit_to_int.js
```

### docs/
**Purpose:** Documentation, implementation notes, and technical details

**Contents:**
- `SUBMODULE_CONVERSION.md` - Documentation of git submodule to flat repository conversion
- `PHASE1_IMPLEMENTATION_SUMMARY.md` - Apple Silicon JIT implementation notes
- `JIT_ASSEMBLY_TRACING_IMPLEMENTATION.md` - JIT assembly tracing details

### examples/
**Purpose:** JavaScript test files and example programs

**Contents:**
- `simple_test.js` - Basic JavaScript execution test
- `jit_demo.js` - JIT compilation demonstration
- `jit_performance_test.js` - Performance benchmarking

### dist/
**Purpose:** Build outputs and distribution packages (generated during build)

**Structure:**
```
dist/
├── chintx64/               # Interpreter-only x86_64 build
├── chjitx64/               # JIT-enabled x86_64 build
├── chinta64/               # Interpreter-only arm64 build
├── chjita64/               # JIT-enabled arm64 build
├── packages/               # Distribution tarballs
└── ChakraSilicon-1.0.0-macos.tar.gz  # Complete release bundle
```

**Note:** This directory is ignored by git (except .gitkeep files). It is created during the build process.

### build/
**Purpose:** CMake build directories (generated during build)

**Structure:**
```
build/
├── chintx64/               # Build files for chintx64 target
├── chjitx64/               # Build files for chjitx64 target
├── chinta64/               # Build files for chinta64 target
└── chjita64/               # Build files for chjita64 target
```

**Note:** This directory is ignored by git. Clean with `./scripts/build_all_and_pack.sh clean`

### examination/
**Purpose:** Investigation and analysis files from development

**Contents:** various files used during development and debugging (e.g., research notes, raw logs).

## Key Files

### VERSION
**Purpose:** Semantic version number for the project. Format: `MAJOR.MINOR.PATCH`.

### STATUS.md
**Purpose:** High-level status of the project, including current blocking issues, recent fixes, and the roadmap.

### README.md
**Purpose:** Main project documentation including quick start, build instructions, and architecture overview.

## Build Targets

The repository supports four primary build targets:

| Target     | Architecture | JIT Mode    | Use Case                          |
|------------|--------------|-------------|-----------------------------------|
| chintx64   | x86_64       | Disabled    | Testing, constrained environments |
| chjitx64   | x86_64       | Enabled     | Production, high performance      |
| chinta64   | arm64        | Disabled    | Apple Silicon testing             |
| chjita64   | arm64        | Enabled     | Apple Silicon production          |

## Contributing

When contributing to this repository:

1. **Scripts:** Add new build scripts to `scripts/` directory. Debugging tools go in `scripts/debug_tools/`.
2. **Tests:** Add reproduction cases to `tests/repro_cases/`.
3. **Documentation:** Add technical docs to `docs/` directory.
4. **Version:** Update `VERSION` and `CHANGELOG.md` for releases.

---

**Last Updated:** February 12, 2026
**Repository Version:** 1.0.0