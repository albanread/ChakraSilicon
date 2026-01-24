#!/bin/bash
#
# build_target.sh - Build a single ChakraCore target variant
#
# Usage:
#   scripts/build_target.sh <target>
#   scripts/build_target.sh list
#
# Targets:
#   chintx64  - interpreter-only, x86_64
#   chjitx64  - JIT-enabled, x86_64
#   chinta64  - interpreter-only, arm64
#   chjita64  - JIT-enabled, arm64
#

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory and repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ChakraCore source directory
CHAKRA_SRC="$REPO_ROOT/ChakraCore"

# Build and output directories
BUILD_BASE="$REPO_ROOT/build"
DIST_BASE="$REPO_ROOT/dist"

# Target definitions (name:arch:mode)
get_target_info() {
    local target=$1
    case $target in
        chintx64) echo "x86_64:interpreter" ;;
        chjitx64) echo "x86_64:jit" ;;
        chinta64) echo "arm64:interpreter" ;;
        chjita64) echo "arm64:jit" ;;
        *) echo "" ;;
    esac
}

ALL_TARGET_NAMES="chintx64 chjitx64 chinta64 chjita64"

# Function to print colored messages
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 <target>
       $0 list

Build a single ChakraCore target variant.

Available targets:
  chintx64  - interpreter-only, x86_64
  chjitx64  - JIT-enabled, x86_64
  chinta64  - interpreter-only, arm64
  chjita64  - JIT-enabled, arm64

Examples:
  $0 chjitx64          # Build JIT-enabled x86_64 variant
  $0 chinta64          # Build interpreter-only arm64 variant
  $0 list              # List all available targets

EOF
}

# Function to list targets
list_targets() {
    echo "Available build targets:"
    echo ""
    for target in $ALL_TARGET_NAMES; do
        local info=$(get_target_info "$target")
        IFS=':' read -r arch mode <<< "$info"
        printf "  %-12s - %-13s %s\n" "$target" "$arch" "($mode)"
    done
    echo ""
}

# Function to detect generator
detect_generator() {
    if command -v ninja &> /dev/null; then
        echo "Ninja"
    else
        echo "Unix Makefiles"
    fi
}

# Function to parse target
parse_target() {
    local target=$1
    local info=$(get_target_info "$target")

    if [[ -z "$info" ]]; then
        log_error "Unknown target: $target"
        echo ""
        list_targets
        exit 1
    fi

    echo "$info"
}

# Function to get CMake architecture flag
get_cmake_arch() {
    local arch=$1
    case "$arch" in
        x86_64)
            echo "x64"
            ;;
        arm64)
            echo "arm64"
            ;;
        *)
            log_error "Unknown architecture: $arch"
            exit 1
            ;;
    esac
}

# Function to build target
build_target() {
    local target=$1
    local arch=$2
    local mode=$3

    log_info "Building target: $target ($arch, $mode)"

    # Determine build directory and output directory
    local build_dir="$BUILD_BASE/$target"
    local dist_dir="$DIST_BASE/$target"

    # Create directories
    mkdir -p "$build_dir"
    mkdir -p "$dist_dir"

    # Determine CMake options
    local cmake_arch=$(get_cmake_arch "$arch")
    local generator=$(detect_generator)

    log_info "Generator: $generator"
    log_info "Architecture: $cmake_arch"
    log_info "Mode: $mode"

    # Set JIT flags based on mode
    local jit_flag=""
    if [[ "$mode" == "interpreter" ]]; then
        jit_flag="-DDISABLE_JIT=ON"
        log_info "JIT: Disabled"
    else
        jit_flag="-DDISABLE_JIT=OFF"
        log_info "JIT: Enabled"
    fi

    # Configure with CMake
    log_info "Configuring CMake..."
    cd "$build_dir"

    cmake "$CHAKRA_SRC" \
        -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES="$arch" \
        -DCMAKE_SYSTEM_PROCESSOR="$cmake_arch" \
        $jit_flag \
        -DCMAKE_INSTALL_PREFIX="$dist_dir" \
        -DBUILD_TESTING=OFF

    # Build
    log_info "Building..."
    if [[ "$generator" == "Ninja" ]]; then
        ninja
    else
        make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
    fi

    # Copy binaries to dist
    log_info "Installing to $dist_dir..."

    # Find and copy the ch binary
    if [[ -f "$build_dir/bin/ch/ch" ]]; then
        cp "$build_dir/bin/ch/ch" "$dist_dir/"
        log_success "Copied ch binary to $dist_dir/ch"
    else
        log_warning "ch binary not found in expected location"
    fi

    # Find and copy ChakraCore library
    if [[ -f "$build_dir/lib/libChakraCore.dylib" ]]; then
        cp "$build_dir/lib/libChakraCore.dylib" "$dist_dir/"
        log_success "Copied libChakraCore.dylib to $dist_dir/"
    elif [[ -f "$build_dir/lib/libChakraCore.so" ]]; then
        cp "$build_dir/lib/libChakraCore.so" "$dist_dir/"
        log_success "Copied libChakraCore.so to $dist_dir/"
    else
        log_warning "ChakraCore library not found in expected location"
    fi

    # Copy any static libraries
    if [[ -f "$build_dir/lib/libChakraCoreStatic.a" ]]; then
        cp "$build_dir/lib/libChakraCoreStatic.a" "$dist_dir/"
        log_success "Copied libChakraCoreStatic.a to $dist_dir/"
    fi

    # Create a build info file
    cat > "$dist_dir/build_info.txt" << EOF
ChakraCore Build Information
============================
Target: $target
Architecture: $arch
Mode: $mode
Build Date: $(date)
Build Host: $(hostname)
CMake Generator: $generator
Build Directory: $build_dir
EOF

    log_success "Build complete for $target"
    log_info "Binaries available in: $dist_dir"

    # Return to repo root
    cd "$REPO_ROOT"
}

# Main script
main() {
    if [[ $# -eq 0 ]]; then
        show_usage
        exit 1
    fi

    local target=$1

    # Handle special commands
    if [[ "$target" == "list" ]]; then
        list_targets
        exit 0
    fi

    if [[ "$target" == "help" ]] || [[ "$target" == "-h" ]] || [[ "$target" == "--help" ]]; then
        show_usage
        exit 0
    fi

    # Check if ChakraCore source exists
    if [[ ! -d "$CHAKRA_SRC" ]]; then
        log_error "ChakraCore source directory not found: $CHAKRA_SRC"
        exit 1
    fi

    # Parse and validate target
    IFS=':' read -r arch mode <<< "$(parse_target "$target")"

    # Build the target
    log_info "Starting build process..."
    echo ""
    build_target "$target" "$arch" "$mode"
    echo ""
    log_success "All done! 🎉"
}

# Run main
main "$@"
