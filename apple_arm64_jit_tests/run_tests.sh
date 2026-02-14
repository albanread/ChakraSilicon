#!/bin/bash
#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

#
# Test runner for Apple ARM64 JIT call bugs
# Tests both Bug A (CallDirect varargs) and Bug B (JS→JS 7+ parameters)
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."

# Find ChakraCore executable
CH_PATH="${PROJECT_ROOT}/ChakraCore/BuildLinux/Debug/ch"
if [ ! -f "$CH_PATH" ]; then
    CH_PATH="${PROJECT_ROOT}/ChakraCore/BuildLinux/Release/ch"
fi
if [ ! -f "$CH_PATH" ]; then
    CH_PATH="${PROJECT_ROOT}/ChakraCore/out/Debug/ch"
fi
if [ ! -f "$CH_PATH" ]; then
    CH_PATH="${PROJECT_ROOT}/ChakraCore/out/Release/ch"
fi
if [ ! -f "$CH_PATH" ]; then
    CH_PATH="${PROJECT_ROOT}/ChakraCore/bin/ch"
fi

# Allow override via environment
CH_PATH="${CHAKRA_BIN:-$CH_PATH}"

# Detect timeout command
TIMEOUT_CMD=""
if command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_CMD="gtimeout 60s"
elif command -v timeout >/dev/null 2>&1; then
    TIMEOUT_CMD="timeout 60s"
fi

echo -e "${BLUE}=== Apple ARM64 JIT Call Bugs Test Suite ===${NC}"
echo ""
echo "Project Root: $PROJECT_ROOT"
echo "ChakraCore Binary: $CH_PATH"
echo ""

# Check if binary exists
if [ ! -f "$CH_PATH" ]; then
    echo -e "${RED}ERROR: ChakraCore binary not found at: $CH_PATH${NC}"
    echo ""
    echo "Please build ChakraCore first or set CHAKRA_BIN environment variable:"
    echo "  export CHAKRA_BIN=/path/to/ch"
    echo ""
    echo "To build ChakraCore:"
    echo "  cd ChakraCore"
    echo "  ./build.sh --debug (or --release)"
    echo ""
    exit 1
fi

# Check architecture
ARCH=$(uname -m)
OS=$(uname -s)
echo "Architecture: $ARCH"
echo "OS: $OS"
echo ""

if [ "$ARCH" != "arm64" ] && [ "$ARCH" != "aarch64" ]; then
    echo -e "${YELLOW}WARNING: Not running on ARM64 architecture!${NC}"
    echo "These tests are designed for Apple Silicon (ARM64)."
    echo "Results may not be meaningful on other architectures."
    echo ""
fi

if [ "$OS" != "Darwin" ]; then
    echo -e "${YELLOW}WARNING: Not running on macOS!${NC}"
    echo "These tests are designed for Apple Silicon (macOS ARM64)."
    echo "Results may not be meaningful on other platforms."
    echo ""
fi

# Test modes
TEST_MODE="${1:-all}"

run_test() {
    local test_name="$1"
    local test_file="$2"
    local jit_flag="$3"
    local description="$4"

    echo -e "${BLUE}--- $test_name ---${NC}"
    echo "Description: $description"
    echo "Test file: $test_file"
    echo "JIT mode: $jit_flag"
    echo ""

    # Run the test
    local run_cmd=()
    if [ -n "$TIMEOUT_CMD" ]; then
        run_cmd+=($TIMEOUT_CMD)
    fi
    run_cmd+=("$CH_PATH")
    if [ -n "$jit_flag" ]; then
        run_cmd+=("$jit_flag")
    fi
    run_cmd+=("$test_file")

    if "${run_cmd[@]}" 2>&1; then
        echo -e "${GREEN}✓ $test_name PASSED${NC}"
        echo ""
        return 0
    else
        local exit_code=$?
        echo -e "${RED}✗ $test_name FAILED (exit code: $exit_code)${NC}"
        echo ""
        return 1
    fi
}

run_test_comparison() {
    local test_name="$1"
    local test_file="$2"
    local description="$3"

    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}Test: $test_name${NC}"
    echo -e "${BLUE}======================================${NC}"
    echo ""

    # Run with JIT disabled (baseline)
    echo -e "${YELLOW}Running with JIT DISABLED (baseline)...${NC}"
    local nojit_output=$(mktemp)
    local base_cmd=()
    if [ -n "$TIMEOUT_CMD" ]; then
        base_cmd+=($TIMEOUT_CMD)
    fi
    base_cmd+=("$CH_PATH" -maxInterpretCount:1 -maxSimpleJitRunCount:1 -bgjit- "$test_file")

    if "${base_cmd[@]}" > "$nojit_output" 2>&1; then
        echo -e "${GREEN}✓ No-JIT run completed${NC}"
        local nojit_passed=$(grep -c "PASS:" "$nojit_output" || echo 0)
        local nojit_failed=$(grep -c "FAIL:" "$nojit_output" || echo 0)
        echo "  Passed: $nojit_passed, Failed: $nojit_failed"
    else
        echo -e "${RED}✗ No-JIT run crashed or failed${NC}"
        cat "$nojit_output"
        rm "$nojit_output"
        return 1
    fi
    echo ""

    # Run with JIT enabled
    echo -e "${YELLOW}Running with JIT ENABLED (testing fix)...${NC}"
    local jit_output=$(mktemp)
    local jit_cmd=()
    if [ -n "$TIMEOUT_CMD" ]; then
        jit_cmd+=($TIMEOUT_CMD)
    fi
    jit_cmd+=("$CH_PATH" "$test_file")

    if "${jit_cmd[@]}" > "$jit_output" 2>&1; then
        echo -e "${GREEN}✓ JIT-enabled run completed${NC}"
        local jit_passed=$(grep -c "PASS:" "$jit_output" || echo 0)
        local jit_failed=$(grep -c "FAIL:" "$jit_output" || echo 0)
        echo "  Passed: $jit_passed, Failed: $jit_failed"
    else
        echo -e "${RED}✗ JIT-enabled run crashed or failed${NC}"
        cat "$jit_output"
        rm "$nojit_output" "$jit_output"
        return 1
    fi
    echo ""

    # Compare results
    echo -e "${BLUE}Comparison:${NC}"
    if [ "$jit_failed" -eq 0 ] && [ "$jit_passed" -gt 0 ]; then
        echo -e "${GREEN}✓✓✓ ALL TESTS PASSED WITH JIT ENABLED ✓✓✓${NC}"
        echo "  No-JIT: $nojit_passed passed, $nojit_failed failed"
        echo "  JIT:    $jit_passed passed, $jit_failed failed"
        echo ""
        rm "$nojit_output" "$jit_output"
        return 0
    else
        echo -e "${RED}✗✗✗ SOME TESTS FAILED WITH JIT ENABLED ✗✗✗${NC}"
        echo "  No-JIT: $nojit_passed passed, $nojit_failed failed"
        echo "  JIT:    $jit_passed passed, $jit_failed failed"
        echo ""
        echo "Full JIT output:"
        cat "$jit_output"
        echo ""
        rm "$nojit_output" "$jit_output"
        return 1
    fi
}

# Run tests based on mode
overall_result=0

case "$TEST_MODE" in
    all)
        echo -e "${BLUE}Running ALL tests${NC}"
        echo ""

        if ! run_test_comparison "Bug A: CallDirect Varargs" \
            "$SCRIPT_DIR/test_calldirect_varargs.js" \
            "Tests C++ variadic Entry* helper calls from JIT"; then
            overall_result=1
        fi

        if ! run_test_comparison "Bug B: JS→JS 7+ Parameters" \
            "$SCRIPT_DIR/test_js_7plus_params.js" \
            "Tests JS-to-JS calls with 7 or more parameters"; then
            overall_result=1
        fi
        ;;

    varargs|bug-a|a)
        echo -e "${BLUE}Running Bug A (CallDirect Varargs) tests only${NC}"
        echo ""

        if ! run_test_comparison "Bug A: CallDirect Varargs" \
            "$SCRIPT_DIR/test_calldirect_varargs.js" \
            "Tests C++ variadic Entry* helper calls from JIT"; then
            overall_result=1
        fi
        ;;

    params|bug-b|b)
        echo -e "${BLUE}Running Bug B (JS→JS 7+ Parameters) tests only${NC}"
        echo ""

        if ! run_test_comparison "Bug B: JS→JS 7+ Parameters" \
            "$SCRIPT_DIR/test_js_7plus_params.js" \
            "Tests JS-to-JS calls with 7 or more parameters"; then
            overall_result=1
        fi
        ;;

    quick)
        echo -e "${BLUE}Running QUICK test (JIT-enabled only)${NC}"
        echo ""

        if ! run_test "Quick: CallDirect Varargs" \
            "$SCRIPT_DIR/test_calldirect_varargs.js" \
            "" \
            "Quick test of CallDirect varargs with JIT"; then
            overall_result=1
        fi

        if ! run_test "Quick: JS→JS 7+ Parameters" \
            "$SCRIPT_DIR/test_js_7plus_params.js" \
            "" \
            "Quick test of 7+ param calls with JIT"; then
            overall_result=1
        fi
        ;;

    help|--help|-h)
        echo "Usage: $0 [MODE]"
        echo ""
        echo "Modes:"
        echo "  all           - Run all tests with comparison (default)"
        echo "  varargs|a     - Run Bug A (CallDirect varargs) tests only"
        echo "  params|b      - Run Bug B (JS→JS 7+ params) tests only"
        echo "  quick         - Quick test with JIT enabled only"
        echo "  help          - Show this help message"
        echo ""
        echo "Environment variables:"
        echo "  CHAKRA_BIN    - Path to ChakraCore 'ch' binary"
        echo ""
        exit 0
        ;;

    *)
        echo -e "${RED}ERROR: Unknown test mode: $TEST_MODE${NC}"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac

# Final summary
echo ""
echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}Test Suite Complete${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

if [ $overall_result -eq 0 ]; then
    echo -e "${GREEN}✓✓✓ ALL TEST SUITES PASSED ✓✓✓${NC}"
    echo ""
    echo "The Apple ARM64 JIT call bugs appear to be FIXED!"
    echo ""
else
    echo -e "${RED}✗✗✗ SOME TEST SUITES FAILED ✗✗✗${NC}"
    echo ""
    echo "There are still issues with the Apple ARM64 JIT implementation."
    echo "Please review the test output above for details."
    echo ""
fi

exit $overall_result
