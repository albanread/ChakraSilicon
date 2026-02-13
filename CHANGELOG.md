# Changelog

All notable changes to ChakraSilicon will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-02-14

### Added
- **NEON Phase 2: JIT Build on Apple Silicon & NEON Backend Infrastructure**
  - **JIT now builds and runs on Apple Silicon (macOS arm64)**
    - Fixed `AppleSiliconBuild.cmake` validation error: CMake was checking for C preprocessor defines (`APPLE_SILICON_JIT`, `PROHIBIT_STP_LDP`) as CMake variables; corrected to validate via `CC_APPLE_SILICON` CMake variable instead
    - All 726 NEON correctness tests pass with JIT enabled
    - JIT delivers **70× speedup** on memset loops and **28× speedup** on element-wise arithmetic loops vs interpreter-only build
  - **67 NEON vector opcodes added to `MdOpCodes.h`** — complete instruction set for JIT vectorization:
    - Data Movement: `NEON_DUP`, `NEON_MOVI`, `NEON_MOV`
    - Load/Store: `NEON_LD1`, `NEON_ST1`, `NEON_LDR_Q`, `NEON_STR_Q`
    - Integer Arithmetic: `NEON_ADD`, `NEON_SUB`, `NEON_MUL`, `NEON_NEG`, `NEON_ABS`
    - Float Arithmetic: `NEON_FADD`, `NEON_FSUB`, `NEON_FMUL`, `NEON_FDIV`, `NEON_FNEG`, `NEON_FABS`, `NEON_FSQRT`, `NEON_FMLA`, `NEON_FMLS`
    - Min/Max: `NEON_SMIN`, `NEON_SMAX`, `NEON_UMIN`, `NEON_UMAX`, `NEON_FMIN`, `NEON_FMAX`, `NEON_FMINNM`, `NEON_FMAXNM`
    - Horizontal Reduction: `NEON_ADDV`, `NEON_SMAXV`, `NEON_SMINV`, `NEON_FADDP`, `NEON_FMAXNMV`, `NEON_FMINNMV`
    - Comparison: `NEON_CMEQ`, `NEON_CMGT`, `NEON_CMGE`, `NEON_CMEQ0`, `NEON_FCMEQ`, `NEON_FCMGT`, `NEON_FCMGE`
    - Bitwise Logic: `NEON_AND`, `NEON_ORR`, `NEON_EOR`, `NEON_NOT`, `NEON_BSL`, `NEON_BIC`
    - Shift: `NEON_SHL`, `NEON_SSHR`, `NEON_USHR`
    - Permute/Shuffle: `NEON_REV64`, `NEON_REV32`, `NEON_REV16`, `NEON_EXT`, `NEON_TBL`
    - Type Conversion: `NEON_SCVTF`, `NEON_UCVTF`, `NEON_FCVTZS`, `NEON_FCVTZU`
    - Element Insert/Extract: `NEON_INS`, `NEON_UMOV`, `NEON_DUP_ELEM`
    - Widen/Narrow: `NEON_SXTL`, `NEON_UXTL`, `NEON_XTN`
    - Prefetch: `NEON_PRFM`
  - **All 67 NEON opcodes wired in `EncoderMD.cpp`** to `ARM64NeonEncoder.h` emit functions
    - Used explicit `NeonRegisterParam` / `Arm64SimpleRegisterParam` construction to disambiguate overloaded emitters (SCVTF, UCVTF, FCVTZS, FCVTZU, DUP, INS, UMOV)
    - Vector arrangement sizes inferred from operand types (SIZE_4S for float32/int32, SIZE_2D for float64, SIZE_8H for int16, SIZE_16B for int8/bitwise)
  - **`CHAKRA_NEON_DISABLED` build-time override** added to `NeonAccel.h` for A/B benchmarking
    - Pass `-DCHAKRA_NEON_DISABLED` to compile with NEON runtime helpers disabled while keeping the same binary

### Fixed
- **NEON correctness test Int8Array sentinel bug**: Changed sentinel from `-128` to `-50` in `testLargeIndexOf`; `-128` appeared in `(i % 200)` stored in Int8Array (128 wraps to -128), causing false indexOf matches at index 128 for arrays with size >= 129
- **Fill benchmark false-positive correctness errors**: Float32Array.fill(3.14) and fill(NaN) report "ERROR" in the benchmark due to float32 truncation (`3.14` double != float32 representation) and `NaN !== NaN`; these are test-display issues, not runtime bugs

### Changed
- `NeonAccel.h` — NEON detection guard now respects `CHAKRA_NEON_DISABLED` override macro
- `AppleSiliconBuild.cmake` — Replaced `FATAL_ERROR` checks for undefined C preprocessor defines with CMake-level `CC_APPLE_SILICON` variable check
- `docs/neonStatus.md` — Updated Phase 1 to 100% complete, Phase 2 to ~70%, Phase 3 infrastructure marked as ready

### Performance (Phase 2 JIT Benchmarks — Apple Silicon M-series)

| Test | Interpreter (NEON ON) | JIT (NEON ON) | Speedup |
|------|----------------------|---------------|---------|
| Memset loop (1M elements × 1000 iters) | 700 ms | 10 ms | **70×** |
| Add loop (1K elements × 5000 iters) | 196 ms | 7 ms | **28×** |

JIT path: GlobOpt detects fill loop → emits `Js::OpCode::Memset` → `Lowerer::LowerMemset` → `HelperOp_Memset` → `OP_Memset` → `DirectSetItemAtRange` → NEON fill intrinsics.

---

## [1.2.0] - 2026-02-13

### Added
- **NEON Phase 1 Completion: All 12 SIMD files ported, TypedArray call-sites wired**
  - Ported remaining 8 SIMD operation files to NEON intrinsics (scalar fallbacks preserved):
    - `SimdUint32x4Operation.cpp` — 10 ops (OpUint32x4, Splat, ShiftRight, Min, Max, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual; OpFromFloat32x4 kept scalar for range-check semantics)
    - `SimdInt16x8Operation.cpp` — 18 ops (Add, Sub, Mul, Neg, Not, And, Or, Xor, AddSaturate, SubSaturate, Min, Max, comparisons, shifts, Splat)
    - `SimdInt8x16Operation.cpp` — 15 ops (Add, Sub, Mul, Neg, AddSaturate, SubSaturate, Min, Max, comparisons, shifts, Splat, Select via `vbslq_u8`)
    - `SimdUint16x8Operation.cpp` — 10 ops (OpUint16x8, Min, Max, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual, ShiftRight, AddSaturate, SubSaturate)
    - `SimdUint8x16Operation.cpp` — 10 ops (OpUint8x16, Min, Max, LessThan, LessThanOrEqual, GreaterThan, GreaterThanOrEqual, ShiftRight, AddSaturate, SubSaturate)
    - `SimdBool32x4Operation.cpp` — 3 ops (OpBool32x4 constructor, OpAnyTrue, OpAllTrue with NEON horizontal reduce)
    - `SimdBool16x8Operation.cpp` — 2 ops (OpBool16x8 constructor, copy overload)
    - `SimdBool8x16Operation.cpp` — 2 ops (OpBool8x16 constructor, copy overload)
  - Wired `TypedArrayBase::FindMinOrMax` to NEON min/max scan helpers for all 8 element types:
    - Float32Array → `NeonMinMaxFloat32` (NaN detection, -0/+0 fixup per JS spec)
    - Float64Array → `NeonMinMaxFloat64` (NaN detection, -0/+0 fixup per JS spec)
    - Int32Array → `NeonMinMaxInt32`, Uint32Array → `NeonMinMaxUint32`
    - Int16Array → `NeonMinMaxInt16`, Uint16Array → `NeonMinMaxUint16`
    - Int8Array → `NeonMinMaxInt8`, Uint8Array/Uint8ClampedArray → `NeonMinMaxUint8`
  - Wired `JavascriptNativeIntArray::HeadSegmentIndexOfHelper` with NEON vectorized search (`vceqq_s32`, 4 elements per cycle, with `static_assert` on `Field(int32)` size for safety)
  - Wired `JavascriptArray::CopyValueToSegmentBuferNoCheck<double>` to `NeonFillFloat64` for non-zero fill path
  - Wired `JavascriptArray::CopyValueToSegmentBuferNoCheck<int32>` to `NeonFillInt32` for non-zero fill path
  - Added `<type_traits>` include to `NeonAccel.h` for `std::is_signed` used in `FindMinOrMax` type dispatch

### Changed
- `SimdUint32x4Operation.cpp` — NEON intrinsics replace 4-scalar-op patterns (4x speedup expected)
- `SimdInt16x8Operation.cpp` — NEON intrinsics replace 8-scalar-op patterns (8x speedup expected)
- `SimdInt8x16Operation.cpp` — NEON intrinsics replace 16-scalar-op patterns (16x speedup expected)
- `SimdUint16x8Operation.cpp` — NEON intrinsics replace 8-scalar-op patterns (8x speedup expected)
- `SimdUint8x16Operation.cpp` — NEON intrinsics replace 16-scalar-op patterns (16x speedup expected)
- `SimdBool32x4Operation.cpp` — NEON horizontal reduce for AnyTrue/AllTrue (4x speedup expected)
- `SimdBool16x8Operation.cpp` — NEON load/store for constructors
- `SimdBool8x16Operation.cpp` — NEON load/store for constructors
- `TypedArray.cpp` — `FindMinOrMax` now uses NEON-accelerated min/max scan (4-16x speedup expected depending on element type)
- `JavascriptArray.cpp` — native int array indexOf uses NEON vectorized search; non-zero segment fills use NEON
- `docs/neonStatus.md` — updated to reflect Phase 1 ~95% completion (141/141 SIMD ops, 14/16 call sites)
- Phase 1 overall progress: ~60% → ~95%

## [1.1.0] - 2025-06-14

### Added
- **NEON Phase 1: C++ Runtime NEON Intrinsics** — First phase of the NEON acceleration plan
  - Created `lib/Runtime/Language/NeonAccel.h` — header-only NEON utility library with:
    - Fill operations for all 8 TypedArray element types (Float32, Float64, Int32, Uint32, Int16, Uint16, Int8, Uint8)
    - IndexOf/search operations for all element types with NEON vector compare and early-exit
    - Min/Max scan operations with correct NaN propagation and -0/+0 handling for floats
    - Array reverse operations using NEON shuffle/rev instructions
    - Bulk copy helpers using NEON load/store pairs
    - SIMD operation helpers for SimdXxxOperation backends
    - Bool operation helpers for AnyTrue/AllTrue
  - Replaced scalar ARM paths with NEON intrinsics in first 4 SIMD operation files:
    - `SimdFloat32x4Operation.cpp` — all ops (Add, Sub, Mul, Div, Abs, Neg, Sqrt, comparisons, Select, Clamp, etc.)
    - `SimdFloat64x2Operation.cpp` — all ops (Add, Sub, Mul, Div, Abs, Neg, Sqrt, comparisons, Select, etc.)
    - `SimdInt32x4Operation.cpp` — all ops (Add, Sub, Mul, And, Or, Xor, Not, Min, Max, shifts, comparisons, Select)
    - `SimdInt64x2Operation.cpp` — all ops (Add, Sub, Neg, Splat, shifts)
  - Added NEON-accelerated fill path to `TypedArray.h` `DirectSetItemAtRange` for non-zero fills (4-byte, 8-byte, 2-byte element types)
  - All NEON code gated on `CHAKRA_NEON_AVAILABLE` with scalar fallbacks preserved
- **Benchmark suite** in `tests/neon_benchmarks/`:
  - `fill_bench.js` — TypedArray.fill() across all types, zero/non-zero, partial, small arrays, special values
  - `indexof_bench.js` — TypedArray.indexOf()/includes() with worst/best/not-found/edge cases
  - `add_bench.js` — element-wise arithmetic loops, unary ops, reductions, copy, reverse, FMA, clamp patterns

### Changed
- `SimdFloat32x4Operation.cpp` — NEON intrinsics replace 4-scalar-op patterns (4x speedup expected)
- `SimdFloat64x2Operation.cpp` — NEON intrinsics replace 2-scalar-op patterns (2x speedup expected)
- `SimdInt32x4Operation.cpp` — NEON intrinsics replace 4-scalar-op patterns (4x speedup expected)
- `SimdInt64x2Operation.cpp` — NEON intrinsics replace 2-scalar-op patterns (2x speedup expected)
- `TypedArray.h` — non-zero fills for 2/4/8-byte element types now dispatch to NEON fill (4-16x speedup expected)

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