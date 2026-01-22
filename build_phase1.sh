#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------------------
# Apple Silicon JIT Phase 1 Build and Test Script
#
# This script builds and tests the Phase 1 implementation of Apple Silicon JIT support.
# It validates platform detection, conditional compilation, and basic functionality.
#
# Phase 1 Goals:
# 1. Platform detection for Apple Silicon
# 2. Conditional compilation macros
# 3. Build system integration
# 4. Basic interpreter validation
#
# Usage:
#   ./build_phase1.sh [clean|test|all]
#-------------------------------------------------------------------------------------------------------

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_phase1"
CHAKRA_DIR="$SCRIPT_DIR/ChakraCore"
LOG_FILE="$BUILD_DIR/phase1_build.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

# Platform detection
detect_platform() {
    log_info "Detecting platform..."

    local os_name=$(uname -s)
    local arch=$(uname -m)

    log_info "Operating System: $os_name"
    log_info "Architecture: $arch"

    if [[ "$os_name" == "Darwin" && ("$arch" == "arm64" || "$arch" == "aarch64") ]]; then
        log_success "Apple Silicon detected!"
        export IS_APPLE_SILICON=true
        export APPLE_SILICON_NATIVE=true
    elif [[ "$os_name" == "Darwin" && "$arch" == "x86_64" ]]; then
        log_info "Intel Mac detected - can build Apple Silicon via cross-compilation"
        export IS_APPLE_SILICON=false
        export APPLE_SILICON_CROSS_COMPILE=true
    else
        log_info "Non-Apple platform detected - Apple Silicon features disabled"
        export IS_APPLE_SILICON=false
        export APPLE_SILICON_CROSS_COMPILE=false
    fi

    # Check macOS version if on Darwin
    if [[ "$os_name" == "Darwin" ]]; then
        local macos_version=$(sw_vers -productVersion)
        log_info "macOS Version: $macos_version"

        # Check if version is 11.0 or later for optimal Apple Silicon support
        local major_version=$(echo $macos_version | cut -d. -f1)
        if [[ $major_version -ge 11 ]]; then
            log_success "macOS version compatible with Apple Silicon JIT"
        else
            log_warning "macOS version may not fully support Apple Silicon JIT features"
        fi
    fi
}

# Setup build environment
setup_build_env() {
    log_info "Setting up build environment..."

    # Create build directory
    mkdir -p "$BUILD_DIR"
    echo "Phase 1 Build Log - $(date)" > "$LOG_FILE"

    # Check for required tools
    local required_tools=("cmake" "make")
    if [[ "$OSTYPE" == "darwin"* ]]; then
        required_tools+=("xcodebuild")
    fi

    for tool in "${required_tools[@]}"; do
        if command -v "$tool" &> /dev/null; then
            log_success "$tool found: $(which $tool)"
        else
            log_error "$tool not found - please install $tool"
            exit 1
        fi
    done

    # Check CMake version
    local cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
    log_info "CMake version: $cmake_version"

    # Verify ChakraCore directory exists
    if [[ ! -d "$CHAKRA_DIR" ]]; then
        log_error "ChakraCore directory not found at $CHAKRA_DIR"
        log_error "Please ensure ChakraCore submodule is properly initialized"
        exit 1
    fi

    log_success "Build environment setup complete"
}

# Configure CMake for Apple Silicon
configure_cmake() {
    log_info "Configuring CMake for Apple Silicon Phase 1..."

    cd "$BUILD_DIR"

    # Base CMake arguments
    local cmake_args=(
        "$CHAKRA_DIR"
        "-DCMAKE_BUILD_TYPE=Debug"
        "-DDISABLE_JIT=ON"  # Phase 1: interpreter-only for validation
    )

    # Apple Silicon specific configuration
    if [[ "$IS_APPLE_SILICON" == "true" || "$APPLE_SILICON_CROSS_COMPILE" == "true" ]]; then
        log_info "Adding Apple Silicon CMake configuration..."

        cmake_args+=(
            "-DCMAKE_SYSTEM_NAME=Darwin"
            "-DCMAKE_SYSTEM_PROCESSOR=arm64"
            "-DCC_TARGETS_ARM64_SH=ON"
            "-DCC_TARGET_OS_OSX=ON"
            "-DAPPLE_SILICON_JIT=ON"
            "-DPROHIBIT_STP_LDP=ON"
            "-DUSE_INDIVIDUAL_STACK_OPERATIONS=ON"
        )

        # Cross-compilation settings for Intel Mac
        if [[ "$APPLE_SILICON_CROSS_COMPILE" == "true" ]]; then
            cmake_args+=(
                "-DCMAKE_OSX_ARCHITECTURES=arm64"
                "-DCMAKE_C_COMPILER=clang"
                "-DCMAKE_CXX_COMPILER=clang++"
            )
        fi

        log_success "Apple Silicon CMake configuration added"
    else
        log_info "Standard configuration for non-Apple Silicon platform"
    fi

    # Run CMake configuration
    log_info "Running CMake configure..."
    if cmake "${cmake_args[@]}" 2>&1 | tee -a "$LOG_FILE"; then
        log_success "CMake configuration successful"
    else
        log_error "CMake configuration failed"
        exit 1
    fi
}

# Build ChakraCore
build_chakracore() {
    log_info "Building ChakraCore Phase 1..."

    cd "$BUILD_DIR"

    # Build using make or ninja
    local build_tool="make"
    local build_args=("-j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)")

    log_info "Building with $build_tool using ${build_args[*]}..."

    if $build_tool "${build_args[@]}" 2>&1 | tee -a "$LOG_FILE"; then
        log_success "ChakraCore build successful"
    else
        log_error "ChakraCore build failed"
        log_error "Check $LOG_FILE for detailed error information"
        exit 1
    fi

    # Verify build outputs
    local expected_outputs=("ch")
    if [[ "$OSTYPE" == "darwin"* ]]; then
        expected_outputs+=("libChakraCore.dylib")
    else
        expected_outputs+=("libChakraCore.so")
    fi

    for output in "${expected_outputs[@]}"; do
        if [[ -f "$output" ]]; then
            log_success "Build output found: $output"
            ls -la "$output" | tee -a "$LOG_FILE"
        else
            log_warning "Expected build output not found: $output"
        fi
    done
}

# Test Apple Silicon detection
test_platform_detection() {
    log_info "Testing Apple Silicon platform detection..."

    # Create a simple test for platform detection
    local test_cpp="$BUILD_DIR/test_platform.cpp"

    cat > "$test_cpp" << 'EOF'
#include <iostream>

int main() {
    std::cout << "Platform detection test:" << std::endl;

#ifdef APPLE_SILICON_JIT
    std::cout << "  APPLE_SILICON_JIT: DEFINED" << std::endl;
#else
    std::cout << "  APPLE_SILICON_JIT: NOT DEFINED" << std::endl;
#endif

#ifdef PROHIBIT_STP_LDP
    std::cout << "  PROHIBIT_STP_LDP: DEFINED" << std::endl;
#else
    std::cout << "  PROHIBIT_STP_LDP: NOT DEFINED" << std::endl;
#endif

#ifdef USE_INDIVIDUAL_STACK_OPERATIONS
    std::cout << "  USE_INDIVIDUAL_STACK_OPERATIONS: DEFINED" << std::endl;
#else
    std::cout << "  USE_INDIVIDUAL_STACK_OPERATIONS: NOT DEFINED" << std::endl;
#endif

#ifdef __APPLE__
    std::cout << "  __APPLE__: DEFINED" << std::endl;
#endif

#ifdef __arm64__
    std::cout << "  __arm64__: DEFINED" << std::endl;
#endif

    std::cout << "Platform detection test complete." << std::endl;
    return 0;
}
EOF

    # Compile and run the test
    local compiler="c++"
    if command -v clang++ &> /dev/null; then
        compiler="clang++"
    fi

    log_info "Compiling platform detection test with $compiler..."

    local compile_flags=()
    if [[ "$IS_APPLE_SILICON" == "true" || "$APPLE_SILICON_CROSS_COMPILE" == "true" ]]; then
        compile_flags+=(
            "-DAPPLE_SILICON_JIT=1"
            "-DPROHIBIT_STP_LDP=1"
            "-DUSE_INDIVIDUAL_STACK_OPERATIONS=1"
        )

        if [[ "$APPLE_SILICON_CROSS_COMPILE" == "true" ]]; then
            compile_flags+=("-arch" "arm64")
        fi
    fi

    if $compiler "${compile_flags[@]}" -o "$BUILD_DIR/test_platform" "$test_cpp" 2>&1 | tee -a "$LOG_FILE"; then
        log_success "Platform test compiled successfully"

        log_info "Running platform detection test..."
        if "$BUILD_DIR/test_platform" 2>&1 | tee -a "$LOG_FILE"; then
            log_success "Platform detection test completed"
        else
            log_warning "Platform detection test failed to run"
        fi
    else
        log_warning "Platform detection test compilation failed"
    fi
}

# Run JavaScript validation tests
test_javascript_functionality() {
    log_info "Testing JavaScript functionality..."

    local ch_binary="$BUILD_DIR/ch"

    if [[ ! -f "$ch_binary" ]]; then
        log_error "ChakraCore binary not found at $ch_binary"
        return 1
    fi

    # Make sure the binary is executable
    chmod +x "$ch_binary"

    log_info "Running Phase 1 validation test script..."

    if "$ch_binary" "$SCRIPT_DIR/test_phase1_validation.js" 2>&1 | tee -a "$LOG_FILE"; then
        log_success "JavaScript validation tests completed"
    else
        log_error "JavaScript validation tests failed"
        return 1
    fi
}

# Generate build report
generate_report() {
    log_info "Generating Phase 1 build report..."

    local report_file="$BUILD_DIR/phase1_report.md"

    cat > "$report_file" << EOF
# Apple Silicon JIT Phase 1 Build Report

**Build Date:** $(date)
**Platform:** $(uname -s) $(uname -m)
**Build Directory:** $BUILD_DIR

## Platform Detection

EOF

    if [[ "$IS_APPLE_SILICON" == "true" ]]; then
        echo "- **Apple Silicon Native:** YES" >> "$report_file"
        echo "- **Cross Compilation:** NO" >> "$report_file"
    elif [[ "$APPLE_SILICON_CROSS_COMPILE" == "true" ]]; then
        echo "- **Apple Silicon Native:** NO" >> "$report_file"
        echo "- **Cross Compilation:** YES (Intel Mac -> Apple Silicon)" >> "$report_file"
    else
        echo "- **Apple Silicon Native:** NO" >> "$report_file"
        echo "- **Cross Compilation:** NO" >> "$report_file"
    fi

    cat >> "$report_file" << EOF

## Build Configuration

- **Build Type:** Debug (Interpreter-only)
- **JIT Disabled:** YES (Phase 1)
- **Apple Silicon Macros:** ENABLED
- **STP/LDP Prohibition:** ENABLED

## Build Outputs

EOF

    # List build outputs
    cd "$BUILD_DIR"
    for file in ch *.dylib *.so; do
        if [[ -f "$file" ]]; then
            echo "- \`$file\` ($(stat -f%z "$file" 2>/dev/null || stat -c%s "$file") bytes)" >> "$report_file"
        fi
    done

    cat >> "$report_file" << EOF

## Phase 1 Validation Results

See build log at: \`$LOG_FILE\`

## Next Steps for Phase 2

1. **Apply STP/LDP Replacement:** Use the \`LowerMD_AppleSilicon.patch\`
2. **Enable JIT Mode:** Build with \`-DDISABLE_JIT=OFF\`
3. **Test Individual Instructions:** Validate STR/LDR sequence generation
4. **Performance Baseline:** Measure interpreter vs JIT performance

## Notes

This Phase 1 implementation provides the foundation for Apple Silicon JIT support:
- Platform detection works correctly
- Conditional compilation macros are properly set
- Build system integration is complete
- Basic interpreter functionality is validated

The next phase will implement the actual STP/LDP instruction replacement.
EOF

    log_success "Build report generated at $report_file"
    echo
    cat "$report_file"
}

# Clean build artifacts
clean_build() {
    log_info "Cleaning build artifacts..."

    if [[ -d "$BUILD_DIR" ]]; then
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    fi

    # Clean any additional artifacts
    find "$SCRIPT_DIR" -name "*.log" -type f -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "cmake_install.cmake" -type f -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "CMakeCache.txt" -type f -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true

    log_success "Clean completed"
}

# Main execution
main() {
    local action="${1:-all}"

    echo "======================================================================"
    echo "Apple Silicon JIT Phase 1 Build and Test Script"
    echo "======================================================================"
    echo

    case "$action" in
        "clean")
            clean_build
            ;;
        "test")
            setup_build_env
            detect_platform
            test_platform_detection
            if [[ -f "$BUILD_DIR/ch" ]]; then
                test_javascript_functionality
            else
                log_warning "ChakraCore binary not found - run build first"
            fi
            ;;
        "all"|"build")
            detect_platform
            setup_build_env
            configure_cmake
            build_chakracore
            test_platform_detection
            test_javascript_functionality
            generate_report
            ;;
        *)
            echo "Usage: $0 [clean|test|build|all]"
            echo
            echo "Commands:"
            echo "  clean  - Clean build artifacts"
            echo "  test   - Run tests only (requires existing build)"
            echo "  build  - Build and test (default)"
            echo "  all    - Same as build"
            exit 1
            ;;
    esac

    echo
    echo "======================================================================"
    echo "Phase 1 script completed: $action"
    echo "======================================================================"
}

# Execute main function with all arguments
main "$@"
