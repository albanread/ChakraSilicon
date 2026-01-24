# Changelog

All notable changes to ChakraSilicon will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-01-24

### Added
- Initial release of ChakraSilicon build system
- Comprehensive build scripts for multiple target variants
- `scripts/build_target.sh` - Build individual target variants (chintx64, chjitx64, chinta64, chjita64)
- `scripts/build_all_and_pack.sh` - Orchestrator to build all targets and create distribution packages
- Support for four build targets:
  - `chintx64` - Interpreter-only x86_64
  - `chjitx64` - JIT-enabled x86_64
  - `chinta64` - Interpreter-only arm64 (Apple Silicon)
  - `chjita64` - JIT-enabled arm64 (Apple Silicon)
- Automated packaging with per-target tarballs and release bundles
- SHA256 checksum generation for all packages
- Organized repository structure:
  - `scripts/` for build automation
  - `docs/` for documentation
  - `examples/` for JavaScript test files
  - `legacy/` for archived build scripts
  - `dist/` for build outputs
- VERSION file for version tracking
- Comprehensive README with usage examples
- CHANGELOG for tracking changes

### Changed
- Converted repository from git submodules to flat structure
- Integrated ChakraCore source code directly (no longer a submodule)
- Integrated capstone source code directly (no longer a nested submodule)
- Reorganized file structure for better maintainability
- Moved documentation files to `docs/` directory
- Moved test/example files to `examples/` directory
- Moved old build scripts to `legacy/` directory
- Updated .gitignore for new structure

### Removed
- Git submodule configuration (`.gitmodules`)
- Submodule tracking for ChakraCore and capstone
- Root-level test and documentation files (moved to organized directories)

### Technical Details
- ChakraCore source: Microsoft/ChakraCore (integrated at specific commit)
- Capstone source: aquynh/capstone (integrated as dependency)
- Build system: CMake with Ninja or Unix Makefiles
- Supported platforms: macOS (Apple Silicon and Intel), Linux (x86_64 and arm64)
- Build type: Release (optimized)

### Documentation
- Added SUBMODULE_CONVERSION.md documenting the submodule-to-flat conversion
- Added comprehensive build and usage documentation in README.md
- Preserved existing implementation documentation:
  - PHASE1_IMPLEMENTATION_SUMMARY.md
  - JIT_ASSEMBLY_TRACING_IMPLEMENTATION.md
  - FINAL_BUILD_REPORT.md

---

## [Unreleased]

### Planned
- Universal binary support for macOS (combining x86_64 and arm64)
- Linux distribution packages (.deb, .rpm)
- Continuous integration setup
- Performance benchmarking suite
- Additional architecture support (if requested)

---

**Note:** This is the first organized release of ChakraSilicon. Previous work was development/experimental builds without formal versioning.