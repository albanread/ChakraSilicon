#!/usr/bin/env bash
#
# build_all_and_pack.sh - Build all ChakraCore targets and create distribution packages
#
# Usage:
#   scripts/build_all_and_pack.sh [all|targets...]
#   scripts/build_all_and_pack.sh clean
#
# Examples:
#   scripts/build_all_and_pack.sh all              # Build all targets and package
#   scripts/build_all_and_pack.sh chjitx64         # Build only chjitx64 and package
#   scripts/build_all_and_pack.sh chjitx64 chinta64  # Build specific targets
#   scripts/build_all_and_pack.sh clean            # Clean build and dist directories
#

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script directory and repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Build script location
BUILD_SCRIPT="$SCRIPT_DIR/build_target.sh"

# Directories
BUILD_BASE="$REPO_ROOT/build"
DIST_BASE="$REPO_ROOT/dist"
PACKAGES_DIR="$DIST_BASE/packages"

# All available targets
ALL_TARGETS=(chintx64 chjitx64 chinta64 chjita64)

# Version info
VERSION_FILE="$REPO_ROOT/VERSION"
if [[ -f "$VERSION_FILE" ]]; then
    VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')
else
    VERSION="dev-$(date +%Y%m%d)"
fi

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

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [all|targets...]
       $0 clean

Build all or specific ChakraCore targets and create distribution packages.

Options:
  all              Build all targets (chintx64, chjitx64, chinta64, chjita64)
  clean            Clean build and dist directories
  <target>...      Build specific targets only

Examples:
  $0 all                    # Build all targets and create packages
  $0 chjitx64               # Build only chjitx64 target
  $0 chjitx64 chinta64      # Build chjitx64 and chinta64 targets
  $0 clean                  # Clean all build artifacts

Available targets:
  chintx64  - interpreter-only, x86_64
  chjitx64  - JIT-enabled, x86_64
  chinta64  - interpreter-only, arm64
  chjita64  - JIT-enabled, arm64

Output:
  dist/<target>/           - Individual target binaries
  dist/packages/<target>.tar.gz  - Per-target tarballs
  dist/ChakraSilicon-<version>-<platform>.tar.gz  - Complete release bundle

EOF
}

# Function to clean build artifacts
clean_all() {
    log_step "Cleaning build artifacts..."

    if [[ -d "$BUILD_BASE" ]]; then
        log_info "Removing $BUILD_BASE"
        rm -rf "$BUILD_BASE"
    fi

    if [[ -d "$DIST_BASE" ]]; then
        log_info "Removing $DIST_BASE"
        rm -rf "$DIST_BASE"
    fi

    log_success "Clean complete!"
}

# Function to build a single target
build_single_target() {
    local target=$1

    log_step "Building target: $target"
    echo ""

    if [[ ! -x "$BUILD_SCRIPT" ]]; then
        log_error "Build script not found or not executable: $BUILD_SCRIPT"
        exit 1
    fi

    "$BUILD_SCRIPT" "$target"

    echo ""
}

# Function to create tarball for a single target
create_target_tarball() {
    local target=$1
    local target_dir="$DIST_BASE/$target"

    if [[ ! -d "$target_dir" ]]; then
        log_warning "Target directory not found: $target_dir"
        return 1
    fi

    log_info "Creating tarball for $target..."

    mkdir -p "$PACKAGES_DIR"

    local tarball="$PACKAGES_DIR/${target}-${VERSION}.tar.gz"

    # Create tarball
    cd "$DIST_BASE"
    tar -czf "$tarball" "$target"
    cd "$REPO_ROOT"

    # Create checksum
    if command -v shasum &> /dev/null; then
        shasum -a 256 "$tarball" > "${tarball}.sha256"
        log_success "Created $tarball"
        log_info "SHA256: $(cat ${tarball}.sha256 | awk '{print $1}')"
    else
        log_success "Created $tarball"
    fi
}

# Function to create complete release bundle
create_release_bundle() {
    log_step "Creating release bundle..."

    # Determine platform
    local platform="unknown"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        platform="macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        platform="linux"
    fi

    local bundle_name="ChakraSilicon-${VERSION}-${platform}"
    local bundle_tarball="$DIST_BASE/${bundle_name}.tar.gz"
    local bundle_dir="$DIST_BASE/.bundle_temp"

    # Create temporary bundle directory
    mkdir -p "$bundle_dir/$bundle_name"

    # Copy all target directories
    for target in "${ALL_TARGETS[@]}"; do
        if [[ -d "$DIST_BASE/$target" ]]; then
            log_info "Adding $target to bundle..."
            cp -r "$DIST_BASE/$target" "$bundle_dir/$bundle_name/"
        fi
    done

    # Copy documentation
    if [[ -f "$REPO_ROOT/README.md" ]]; then
        cp "$REPO_ROOT/README.md" "$bundle_dir/$bundle_name/"
    fi

    if [[ -f "$REPO_ROOT/LICENSE" ]]; then
        cp "$REPO_ROOT/LICENSE" "$bundle_dir/$bundle_name/"
    elif [[ -f "$REPO_ROOT/ChakraCore/LICENSE.txt" ]]; then
        cp "$REPO_ROOT/ChakraCore/LICENSE.txt" "$bundle_dir/$bundle_name/"
    fi

    # Create release info
    cat > "$bundle_dir/$bundle_name/RELEASE_INFO.txt" << EOF
ChakraSilicon Release Bundle
============================
Version: $VERSION
Platform: $platform
Build Date: $(date)
Build Host: $(hostname)

Included Targets:
EOF

    for target in "${ALL_TARGETS[@]}"; do
        if [[ -d "$DIST_BASE/$target" ]]; then
            echo "  - $target" >> "$bundle_dir/$bundle_name/RELEASE_INFO.txt"
        fi
    done

    # Create tarball
    cd "$bundle_dir"
    tar -czf "$bundle_tarball" "$bundle_name"
    cd "$REPO_ROOT"

    # Create checksum
    if command -v shasum &> /dev/null; then
        shasum -a 256 "$bundle_tarball" > "${bundle_tarball}.sha256"
        log_success "Created release bundle: $bundle_tarball"
        log_info "SHA256: $(cat ${bundle_tarball}.sha256 | awk '{print $1}')"
    else
        log_success "Created release bundle: $bundle_tarball"
    fi

    # Cleanup temp directory
    rm -rf "$bundle_dir"
}

# Function to print summary
print_summary() {
    local targets=("$@")

    echo ""
    echo "=========================================="
    log_step "Build Summary"
    echo "=========================================="
    echo ""

    log_info "Version: $VERSION"
    log_info "Built targets: ${#targets[@]}"
    echo ""

    for target in "${targets[@]}"; do
        if [[ -d "$DIST_BASE/$target" ]]; then
            echo -e "  ${GREEN}✓${NC} $target"
            if [[ -f "$DIST_BASE/$target/ch" ]]; then
                local size=$(du -sh "$DIST_BASE/$target/ch" | awk '{print $1}')
                echo "      Binary size: $size"
            fi
        else
            echo -e "  ${RED}✗${NC} $target"
        fi
    done

    echo ""
    log_info "Artifacts:"

    if [[ -d "$PACKAGES_DIR" ]]; then
        echo "  Target packages:"
        for tarball in "$PACKAGES_DIR"/*.tar.gz; do
            if [[ -f "$tarball" ]]; then
                local size=$(du -sh "$tarball" | awk '{print $1}')
                echo "    - $(basename $tarball) ($size)"
            fi
        done
    fi

    echo ""
    echo "  Release bundles:"
    for bundle in "$DIST_BASE"/ChakraSilicon-*.tar.gz; do
        if [[ -f "$bundle" ]]; then
            local size=$(du -sh "$bundle" | awk '{print $1}')
            echo "    - $(basename $bundle) ($size)"
        fi
    done

    echo ""
    echo "=========================================="
    log_success "All done! 🚀"
    echo "=========================================="
    echo ""
}

# Main script
main() {
    echo ""
    echo "=========================================="
    echo "  ChakraSilicon Build Orchestrator"
    echo "  Version: $VERSION"
    echo "=========================================="
    echo ""

    if [[ $# -eq 0 ]]; then
        show_usage
        exit 1
    fi

    # Parse arguments
    local targets_to_build=()
    local do_clean=0

    for arg in "$@"; do
        case "$arg" in
            clean)
                do_clean=1
                ;;
            all)
                targets_to_build=("${ALL_TARGETS[@]}")
                ;;
            help|-h|--help)
                show_usage
                exit 0
                ;;
            *)
                targets_to_build+=("$arg")
                ;;
        esac
    done

    # Handle clean
    if [[ $do_clean -eq 1 ]]; then
        clean_all
        exit 0
    fi

    # Validate we have targets to build
    if [[ ${#targets_to_build[@]} -eq 0 ]]; then
        log_error "No targets specified"
        show_usage
        exit 1
    fi

    # Check if build script exists
    if [[ ! -f "$BUILD_SCRIPT" ]]; then
        log_error "Build script not found: $BUILD_SCRIPT"
        exit 1
    fi

    # Make build script executable
    chmod +x "$BUILD_SCRIPT"

    log_info "Targets to build: ${targets_to_build[*]}"
    echo ""

    # Build each target
    local built_targets=()
    for target in "${targets_to_build[@]}"; do
        if build_single_target "$target"; then
            built_targets+=("$target")
        else
            log_error "Failed to build $target"
            exit 1
        fi
    done

    # Create packages
    log_step "Creating distribution packages..."
    echo ""

    for target in "${built_targets[@]}"; do
        create_target_tarball "$target"
    done

    echo ""

    # Create release bundle
    create_release_bundle

    # Print summary
    print_summary "${built_targets[@]}"
}

# Run main
main "$@"
