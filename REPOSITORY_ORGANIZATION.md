# Repository Organization

## Overview

This document describes the complete organization structure of the ChakraSilicon repository, explaining the purpose of each directory and how to navigate the codebase.

## Repository Structure

```
ChakraSilicon/
├── ChakraCore/              # Full ChakraCore source code (integrated)
├── scripts/                 # Build and automation scripts
├── docs/                    # Documentation and technical notes
├── examples/                # JavaScript test files and examples
├── legacy/                  # Archived/legacy build scripts
├── dist/                    # Build outputs (generated)
├── build/                   # Build directories (generated)
├── examination/             # Analysis and investigation files
├── VERSION                  # Current version number (semantic versioning)
├── README.md                # Main documentation and quick start guide
├── CHANGELOG.md             # Version history and changes
├── REPOSITORY_ORGANIZATION.md  # This file
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

**Usage:**
```bash
# Build a single target
./scripts/build_target.sh chjitx64

# Build all targets and package
./scripts/build_all_and_pack.sh all

# List available targets
./scripts/build_target.sh list
```

**Features:**
- Colored output for better readability
- Automatic generator detection (Ninja or Make)
- Build metadata generation
- SHA256 checksum creation
- Release bundle packaging

### docs/
**Purpose:** Documentation, implementation notes, and technical details

**Contents:**
- `SUBMODULE_CONVERSION.md` - Documentation of git submodule to flat repository conversion
- `PHASE1_IMPLEMENTATION_SUMMARY.md` - Apple Silicon JIT implementation notes
- `JIT_ASSEMBLY_TRACING_IMPLEMENTATION.md` - JIT assembly tracing details
- `FINAL_BUILD_REPORT.md` - Build system final report

**When to Reference:**
- Understanding the repository's history and evolution
- Learning about Apple Silicon JIT implementation
- Debugging build issues
- Contributing to the project

### examples/
**Purpose:** JavaScript test files and example programs

**Contents:**
- `simple_test.js` - Basic JavaScript execution test
- `jit_demo.js` - JIT compilation demonstration
- `jit_performance_test.js` - Performance benchmarking
- `jit_test.js` - JIT functionality tests
- `jit_trace_demo.js` - JIT tracing examples
- `test_phase1_validation.js` - Platform detection validation
- Output files (*.txt, *.trace) - Generated test outputs

**Usage:**
```bash
# Run example with built binary
./dist/chjitx64/ch examples/simple_test.js
./dist/chinta64/ch examples/jit_demo.js
```

### legacy/
**Purpose:** Archived build scripts from earlier development phases

**Contents:**
- `build_phase1.sh` - Phase 1 build script
- `build_phase1_native.sh` - Native architecture build script
- `build_x64_jit.sh` - x86_64 JIT build script

**Note:** These scripts are preserved for historical reference but are superseded by the unified scripts in `scripts/`. Do not use for new builds.

### dist/
**Purpose:** Build outputs and distribution packages (generated during build)

**Structure:**
```
dist/
├── chintx64/               # Interpreter-only x86_64 build
│   ├── ch
│   ├── libChakraCore.dylib
│   ├── libChakraCoreStatic.a
│   └── build_info.txt
├── chjitx64/               # JIT-enabled x86_64 build
├── chinta64/               # Interpreter-only arm64 build
├── chjita64/               # JIT-enabled arm64 build
├── packages/               # Distribution tarballs
│   ├── chintx64-1.0.0.tar.gz
│   ├── chintx64-1.0.0.tar.gz.sha256
│   ├── chjitx64-1.0.0.tar.gz
│   ├── chjitx64-1.0.0.tar.gz.sha256
│   └── ...
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

**Contents:** Various files used during development and debugging

**Note:** This directory contains ad-hoc analysis files and may be cleaned up in future versions.

## Key Files

### VERSION
**Purpose:** Semantic version number for the project

**Format:** `MAJOR.MINOR.PATCH` (e.g., `1.0.0`)

**Usage:** Referenced by build scripts to version output packages

### README.md
**Purpose:** Main project documentation

**Contents:**
- Quick start guide
- Build instructions
- Target descriptions
- Usage examples
- Platform support information

**Audience:** All users (developers, contributors, end-users)

### CHANGELOG.md
**Purpose:** Version history and change tracking

**Format:** Keep a Changelog format with semantic versioning

**Contents:**
- Release notes for each version
- Added, changed, removed, and fixed items
- Planned future features

### .gitignore
**Purpose:** Git ignore rules for build artifacts and temporary files

**Ignores:**
- Build directories (`build/`, `build_*/`)
- Distribution outputs (`dist/*` except structure)
- IDE files (`.vscode/`, `.idea/`)
- OS files (`.DS_Store`, `Thumbs.db`)
- Build artifacts (`.o`, `.a`, `.dylib`, `.so`)
- Archives (`.tar.gz`, `.zip`)

## Build Targets

The repository supports four primary build targets:

| Target     | Architecture | JIT Mode    | Use Case                          |
|------------|--------------|-------------|-----------------------------------|
| chintx64   | x86_64       | Disabled    | Testing, constrained environments |
| chjitx64   | x86_64       | Enabled     | Production, high performance      |
| chinta64   | arm64        | Disabled    | Apple Silicon testing             |
| chjita64   | arm64        | Enabled     | Apple Silicon production          |

## Workflow Examples

### Building a Single Target
```bash
# Navigate to repository root
cd ChakraSilicon

# Build JIT-enabled x86_64 variant
./scripts/build_target.sh chjitx64

# Output will be in dist/chjitx64/
./dist/chjitx64/ch examples/simple_test.js
```

### Building All Targets for Release
```bash
# Build all targets and create distribution packages
./scripts/build_all_and_pack.sh all

# Outputs:
# - dist/chintx64/, dist/chjitx64/, dist/chinta64/, dist/chjita64/
# - dist/packages/*.tar.gz (per-target packages)
# - dist/ChakraSilicon-*.tar.gz (release bundle)
```

### Cleaning Build Artifacts
```bash
# Remove all build and distribution files
./scripts/build_all_and_pack.sh clean
```

## Version Management

### Current Version
Check `VERSION` file for the current version number.

### Updating Version
1. Edit `VERSION` file with new semantic version number
2. Update `CHANGELOG.md` with changes for the new version
3. Commit changes: `git commit -am "Bump version to X.Y.Z"`
4. Tag release: `git tag -a vX.Y.Z -m "Release version X.Y.Z"`
5. Build and distribute: `./scripts/build_all_and_pack.sh all`

## Maintenance

### Regular Cleanup
```bash
# Clean build artifacts (recommended between version updates)
./scripts/build_all_and_pack.sh clean

# Remove old build directories from legacy builds
rm -rf build_chjitx64 build_phase1_native build_x64_jit
```

### Code Organization
- Keep build scripts in `scripts/`
- Keep documentation in `docs/`
- Keep examples in `examples/`
- Archive old scripts in `legacy/` (don't delete history)
- Never commit build outputs (`dist/`, `build/`)

## Contributing

When contributing to this repository:

1. **Scripts:** Add new build scripts to `scripts/` directory
2. **Documentation:** Add technical docs to `docs/` directory
3. **Examples:** Add JavaScript examples to `examples/` directory
4. **Version:** Update `VERSION` and `CHANGELOG.md` for releases
5. **Structure:** Maintain the organized directory structure

## Migration from Old Structure

This repository was recently reorganized from a flat structure to the current organized layout:

**Old Location** → **New Location**
- `build_*.sh` → `scripts/` or `legacy/`
- `*.md` (docs) → `docs/`
- `*.js` (tests) → `examples/`
- Scattered files → Organized directories

See `docs/SUBMODULE_CONVERSION.md` for details on the submodule-to-flat conversion.

## Questions?

- For build issues: Check `docs/FINAL_BUILD_REPORT.md`
- For JIT implementation: Check `docs/JIT_ASSEMBLY_TRACING_IMPLEMENTATION.md`
- For repository history: Check `docs/SUBMODULE_CONVERSION.md`
- For version changes: Check `CHANGELOG.md`
- For usage: Check `README.md`

---

**Last Updated:** January 24, 2025  
**Repository Version:** 1.0.0