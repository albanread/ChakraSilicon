# SiliconChakra

This repository contains helper scripts to build ChakraCore variants for x86_64 and Apple "a64" (mapped to arm64).

New scripts added under `scripts/`:

- `scripts/build_target.sh` — build a single target variant. Targets: `chintx64`, `chjitx64`, `chinta64`, `chjita64`. Use `scripts/build_target.sh list` to see available targets.
- `scripts/build_all_and_pack.sh` — orchestrator to build all targets and package them into `dist/` with per-target tarballs and a release bundle.

Supported targets (convention used by these scripts):
- `chintx64`  : interpreter-only, x86_64
- `chjitx64`  : jit-enabled, x86_64
- `chinta64`  : interpreter-only, arm64 (Apple a64 -> arm64)
- `chjita64`  : jit-enabled, arm64

Usage examples:

- Build a single target (from repository root):
    scripts/build_target.sh chjitx64

- Build all targets and package into `dist/`:
    scripts/build_all_and_pack.sh all

Notes and tips:

- On macOS, "a64" targets map to the `arm64` architecture for CMake.
- The scripts prefer `ninja` as the generator when available; they will fall back to Unix Makefiles if ninja is not installed.
- Ensure you have `cmake` and an appropriate compiler toolchain in PATH. On macOS, `clang`/`clang++` is used.
- If this repo uses submodules, ensure the ChakraCore submodule is initialized:
    git submodule update --init --recursive

Artifacts:

- Built binaries are placed in `dist/` with names matching the targets (for example `dist/chjitx64`).
- `scripts/build_all_and_pack.sh` creates per-target tarballs in `dist/packages/` and a top-level release tarball in `dist/`, along with checksums.

If you want the build scripts made executable:
- On macOS / Linux: run `chmod +x scripts/*.sh`

If you need changes to the build targets, packaging layout, or to produce additional distributions (e.g., zipped macOS app bundles), update the scripts in `scripts/` or ask for an enhancement and I'll propose edits.

