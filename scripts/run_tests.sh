#!/bin/bash
#
# run_tests.sh - Run ChakraCore test suite against baselines for Release builds
#
# Usage:
#   scripts/run_tests.sh [binary] [folder...]
#
# Examples:
#   scripts/run_tests.sh                              # Test chjita64, all folders
#   scripts/run_tests.sh dist/chinta64/ch             # Test interpreter build
#   scripts/run_tests.sh dist/chjita64/ch Basics Array # Test specific folders
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_ROOT="$REPO_ROOT/ChakraCore/test"

# Color support: disable if not a tty or --no-color is passed
USE_COLOR=1
if [[ ! -t 1 ]]; then
    USE_COLOR=0
fi
for arg in "$@"; do
    if [[ "$arg" == "--no-color" ]]; then
        USE_COLOR=0
    fi
done

if [[ $USE_COLOR -eq 1 ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    CYAN='\033[0;36m'
    BOLD='\033[1m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    CYAN=''
    BOLD=''
    NC=''
fi

# Defaults
BINARY=""
FOLDERS=()
TIMEOUT=30
PARALLEL=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
VERBOSE=0
SHOW_FAILS=1

# Parse args
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--timeout)
            TIMEOUT="$2"; shift 2 ;;
        -j|--jobs)
            PARALLEL="$2"; shift 2 ;;
        -v|--verbose)
            VERBOSE=1; shift ;;
        -q|--quiet)
            SHOW_FAILS=0; shift ;;
        --no-color)
            shift ;; # Already handled above
        -h|--help)
            cat <<EOF
Usage: $0 [options] [binary] [folder...]

Options:
  -t, --timeout SEC   Per-test timeout (default: $TIMEOUT)
  -j, --jobs N        Parallel jobs (default: $PARALLEL)
  -v, --verbose       Show each test result
  -q, --quiet         Don't show failure details
  --no-color           Disable color output (auto-disabled when piped)
  -h, --help           Show this help

If no binary is given, uses dist/chjita64/ch.
If no folders are given, runs all test folders.

Examples:
  $0                                    # All tests, JIT build
  $0 dist/chinta64/ch                   # All tests, interpreter build
  $0 dist/chjita64/ch Basics Array ES6  # Specific folders
EOF
            exit 0 ;;
        *)
            if [[ -z "$BINARY" && -f "$1" ]]; then
                BINARY="$1"
            elif [[ -z "$BINARY" && -f "$REPO_ROOT/$1" ]]; then
                BINARY="$REPO_ROOT/$1"
            else
                FOLDERS+=("$1")
            fi
            shift ;;
    esac
done

# Default binary
if [[ -z "$BINARY" ]]; then
    BINARY="$REPO_ROOT/dist/chjita64/ch"
fi

if [[ ! -x "$BINARY" ]]; then
    echo -e "${RED}ERROR:${NC} Binary not found or not executable: $BINARY"
    exit 1
fi

# Default folders: all subdirectories of test/ that contain rlexe.xml
if [[ ${#FOLDERS[@]} -eq 0 ]]; then
    while IFS= read -r -d '' xml; do
        folder="$(dirname "$xml")"
        FOLDERS+=("$(basename "$folder")")
    done < <(find "$TEST_ROOT" -maxdepth 2 -name "rlexe.xml" -print0 | sort -z)
fi

# Counters
TOTAL_PASS=0
TOTAL_FAIL=0
TOTAL_SKIP=0
TOTAL_CRASH=0
TOTAL_TIMEOUT=0
TOTAL_TESTS=0

# Per-folder results
declare -a FOLDER_NAMES
declare -a FOLDER_PASS
declare -a FOLDER_FAIL
declare -a FOLDER_SKIP
declare -a FOLDER_TOTAL

# Failed test details
FAIL_DETAILS=""

# Tags/flags that require non-Release features
SKIP_TAGS="exclude_xplat|require_winglob|require_simd|exclude_mac|Slow|exclude_arm64|require_backend|require_debugger|exclude_noicu|require_icu|exclude_interpreted|fails_interpreted|BugFix|CustomConfigFile|require_disable_jit|exclude_dynapogo|require_asmjs"

# Parse rlexe.xml and run tests for one folder
run_folder() {
    local folder_name="$1"
    local folder_path="$TEST_ROOT/$folder_name"
    local rlexe="$folder_path/rlexe.xml"

    if [[ ! -f "$rlexe" ]]; then
        return
    fi

    local pass=0 fail=0 skip=0 total=0 crashes=0 timeouts=0
    local folder_fail_details=""

    # Parse rlexe.xml with Python for reliability
    local test_entries
    test_entries=$(python3 -c "
import xml.etree.ElementTree as ET
import sys

try:
    tree = ET.parse('$rlexe')
except:
    sys.exit(0)

root = tree.getroot()
for test in root.findall('test'):
    default = test.find('default')
    if default is None:
        continue
    files_el = default.find('files')
    baseline_el = default.find('baseline')
    tags_el = default.find('tags')
    flags_el = default.find('compile-flags')

    files = files_el.text.strip() if files_el is not None and files_el.text else ''
    baseline = baseline_el.text.strip() if baseline_el is not None and baseline_el.text else ''
    tags = tags_el.text.strip() if tags_el is not None and tags_el.text else ''
    flags = flags_el.text.strip() if flags_el is not None and flags_el.text else ''

    if not files:
        continue

    print(f'{files}|||{baseline}|||{tags}|||{flags}')
" 2>/dev/null) || return

    if [[ -z "$test_entries" ]]; then
        return
    fi

    while IFS= read -r entry; do
        local test_file baseline tags flags
        IFS='|||' read -r test_file _sep1 baseline _sep2 tags _sep3 flags <<< "$(echo "$entry" | sed 's/|||/|||/g')"

        # Re-parse properly
        test_file="$(echo "$entry" | awk -F'\\|\\|\\|' '{print $1}')"
        baseline="$(echo "$entry" | awk -F'\\|\\|\\|' '{print $2}')"
        tags="$(echo "$entry" | awk -F'\\|\\|\\|' '{print $3}')"
        flags="$(echo "$entry" | awk -F'\\|\\|\\|' '{print $4}')"

        total=$((total + 1))

        # Skip tests with excluded tags
        if [[ -n "$tags" ]] && echo "$tags" | grep -qiE "$SKIP_TAGS"; then
            skip=$((skip + 1))
            [[ $VERBOSE -eq 1 ]] && echo -e "  ${YELLOW}SKIP${NC}: $folder_name/$test_file (tag: $tags)"
            continue
        fi

        # Skip tests requiring compile flags that Release builds don't support
        # Allow -Intl- (we have Intl), -args/-endargs, -force:*, -ES6*
        if [[ -n "$flags" ]]; then
            local dominated_by_unsupported=0
            # Check each flag
            for flag in $flags; do
                case "$flag" in
                    -Intl-|-Intl) ;; # Fine, we have ICU
                    -args|-endargs) ;; # Argument passing, fine
                    -ES6*|-es6*) ;; # ES6 flags
                    -force:*) dominated_by_unsupported=1 ;; # Requires debug/test
                    -maxInterpretCount:*) dominated_by_unsupported=1 ;;
                    -maxSimpleJitRunCount:*) dominated_by_unsupported=1 ;;
                    -forceNative) dominated_by_unsupported=1 ;;
                    -nonative) dominated_by_unsupported=1 ;;
                    -bgjit*) dominated_by_unsupported=1 ;;
                    -off:*) dominated_by_unsupported=1 ;;
                    -on:*) dominated_by_unsupported=1 ;;
                    -mic:*) dominated_by_unsupported=1 ;;
                    -msjrc:*) dominated_by_unsupported=1 ;;
                    -oopjit*) dominated_by_unsupported=1 ;;
                    -recyclerstress*) dominated_by_unsupported=1 ;;
                    -dynamicprofile*) dominated_by_unsupported=1 ;;
                    -MinMemOpCount:*) dominated_by_unsupported=1 ;;
                    -MaxLinearStringSize:*) dominated_by_unsupported=1 ;;
                    -forceserialized*) dominated_by_unsupported=1 ;;
                    -loopinterpretcount:*) dominated_by_unsupported=1 ;;
                    -simpleJitAfter:*) dominated_by_unsupported=1 ;;
                    -forceundodefer) dominated_by_unsupported=1 ;;
                    -deferparse) dominated_by_unsupported=1 ;;
                    -forcedeferparse) dominated_by_unsupported=1 ;;
                    -ForceStrictMode) dominated_by_unsupported=1 ;;
                    -ForceSplitScope*) dominated_by_unsupported=1 ;;
                    -ForceArrayBTree) dominated_by_unsupported=1 ;;
                    -ForceES5Array) dominated_by_unsupported=1 ;;
                    -MinInterpretCount:*) dominated_by_unsupported=1 ;;
                    -MinSimpleJitRunCount:*) dominated_by_unsupported=1 ;;
                    -MaxTemplatizedJitRunCount:*) dominated_by_unsupported=1 ;;
                    -trace:*) dominated_by_unsupported=1 ;;
                    -dump:*) dominated_by_unsupported=1 ;;
                    -IgnoreScriptError*) dominated_by_unsupported=1 ;;
                    -CollectGarbage) ;; # Maybe supported
                    *) ;; # Unknown flag, try anyway
                esac
            done
            if [[ $dominated_by_unsupported -eq 1 ]]; then
                skip=$((skip + 1))
                [[ $VERBOSE -eq 1 ]] && echo -e "  ${YELLOW}SKIP${NC}: $folder_name/$test_file (flags: $flags)"
                continue
            fi
        fi

        # Check test file exists
        local test_path="$folder_path/$test_file"
        if [[ ! -f "$test_path" ]]; then
            skip=$((skip + 1))
            continue
        fi

        # Run the test
        local actual_output
        actual_output=$(timeout "${TIMEOUT}s" "$BINARY" "$test_path" 2>&1)
        local exit_code=$?

        if [[ $exit_code -eq 124 ]]; then
            # Timeout
            timeouts=$((timeouts + 1))
            fail=$((fail + 1))
            [[ $VERBOSE -eq 1 ]] && echo -e "  ${RED}TIMEOUT${NC}: $folder_name/$test_file"
            folder_fail_details+="  TIMEOUT: $folder_name/$test_file\n"
            continue
        fi

        if [[ $exit_code -gt 128 ]]; then
            # Crashed (signal)
            local sig=$((exit_code - 128))
            crashes=$((crashes + 1))
            fail=$((fail + 1))
            [[ $VERBOSE -eq 1 ]] && echo -e "  ${RED}CRASH${NC}: $folder_name/$test_file (signal $sig)"
            folder_fail_details+="  CRASH (signal $sig): $folder_name/$test_file\n"
            continue
        fi

        # If there's a baseline, compare output
        if [[ -n "$baseline" && -f "$folder_path/$baseline" ]]; then
            local expected_output
            expected_output=$(cat "$folder_path/$baseline")

            # Normalize line endings and trailing whitespace
            local norm_actual norm_expected
            norm_actual=$(echo "$actual_output" | sed 's/[[:space:]]*$//' | sed '/^$/d')
            norm_expected=$(echo "$expected_output" | sed 's/[[:space:]]*$//' | sed '/^$/d')

            if [[ "$norm_actual" == "$norm_expected" ]]; then
                pass=$((pass + 1))
                [[ $VERBOSE -eq 1 ]] && echo -e "  ${GREEN}PASS${NC}: $folder_name/$test_file"
            else
                fail=$((fail + 1))
                [[ $VERBOSE -eq 1 ]] && echo -e "  ${RED}FAIL${NC}: $folder_name/$test_file (baseline mismatch)"
                folder_fail_details+="  FAIL (baseline mismatch): $folder_name/$test_file\n"
            fi
        else
            # No baseline — pass if exit code is 0
            if [[ $exit_code -eq 0 ]]; then
                pass=$((pass + 1))
                [[ $VERBOSE -eq 1 ]] && echo -e "  ${GREEN}PASS${NC}: $folder_name/$test_file"
            else
                fail=$((fail + 1))
                [[ $VERBOSE -eq 1 ]] && echo -e "  ${RED}FAIL${NC}: $folder_name/$test_file (exit code $exit_code)"
                folder_fail_details+="  FAIL (exit $exit_code): $folder_name/$test_file\n"
            fi
        fi
    done <<< "$test_entries"

    # Print folder summary
    local pct=0
    if [[ $((pass + fail)) -gt 0 ]]; then
        pct=$(( (pass * 100) / (pass + fail) ))
    fi

    local color="$GREEN"
    if [[ $fail -gt 0 ]]; then
        color="$RED"
    fi
    if [[ $total -eq $skip ]]; then
        color="$YELLOW"
    fi

    printf "  %-30s ${GREEN}%3d${NC} pass  ${RED}%3d${NC} fail  ${YELLOW}%3d${NC} skip  (%3d total)  ${color}%d%%${NC}\n" \
        "$folder_name" "$pass" "$fail" "$skip" "$total" "$pct"

    # Accumulate
    TOTAL_PASS=$((TOTAL_PASS + pass))
    TOTAL_FAIL=$((TOTAL_FAIL + fail))
    TOTAL_SKIP=$((TOTAL_SKIP + skip))
    TOTAL_CRASH=$((TOTAL_CRASH + crashes))
    TOTAL_TIMEOUT=$((TOTAL_TIMEOUT + timeouts))
    TOTAL_TESTS=$((TOTAL_TESTS + total))

    if [[ -n "$folder_fail_details" ]]; then
        FAIL_DETAILS+="$folder_fail_details"
    fi

    FOLDER_NAMES+=("$folder_name")
    FOLDER_PASS+=("$pass")
    FOLDER_FAIL+=("$fail")
    FOLDER_SKIP+=("$skip")
    FOLDER_TOTAL+=("$total")
}

# Header
echo ""
echo -e "${BOLD}############### ChakraSilicon Test Runner ###############${NC}"
echo -e "Binary:   ${CYAN}$BINARY${NC}"
echo -e "Timeout:  ${TIMEOUT}s per test"
echo -e "Folders:  ${#FOLDERS[@]}"
echo -e "Date:     $(date)"
echo ""
echo -e "${BOLD}Results:${NC}"

# Run all folders
START_TIME=$(date +%s)
for folder in "${FOLDERS[@]}"; do
    run_folder "$folder"
done
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

# Summary
echo ""
echo -e "${BOLD}==================== SUMMARY ====================${NC}"

TOTAL_RAN=$((TOTAL_PASS + TOTAL_FAIL))
if [[ $TOTAL_RAN -gt 0 ]]; then
    OVERALL_PCT=$(( (TOTAL_PASS * 100) / TOTAL_RAN ))
else
    OVERALL_PCT=0
fi

echo -e "  Total tests:    $TOTAL_TESTS"
echo -e "  Ran:            $TOTAL_RAN"
echo -e "  ${GREEN}Passed:         $TOTAL_PASS${NC}"
echo -e "  ${RED}Failed:         $TOTAL_FAIL${NC}"
echo -e "  ${YELLOW}Skipped:        $TOTAL_SKIP${NC}"
if [[ $TOTAL_CRASH -gt 0 ]]; then
    echo -e "  ${RED}Crashes:        $TOTAL_CRASH${NC}"
fi
if [[ $TOTAL_TIMEOUT -gt 0 ]]; then
    echo -e "  ${RED}Timeouts:       $TOTAL_TIMEOUT${NC}"
fi
echo ""

if [[ $OVERALL_PCT -ge 95 ]]; then
    echo -e "  ${GREEN}${BOLD}Pass rate: ${OVERALL_PCT}%${NC}"
elif [[ $OVERALL_PCT -ge 80 ]]; then
    echo -e "  ${YELLOW}${BOLD}Pass rate: ${OVERALL_PCT}%${NC}"
else
    echo -e "  ${RED}${BOLD}Pass rate: ${OVERALL_PCT}%${NC}"
fi

echo -e "  Time: ${ELAPSED}s"
echo -e "${BOLD}====================================================${NC}"

# Show failures
if [[ $SHOW_FAILS -eq 1 && -n "$FAIL_DETAILS" ]]; then
    echo ""
    echo -e "${BOLD}Failed tests:${NC}"
    echo -e "$FAIL_DETAILS"
fi

# Exit code
if [[ $TOTAL_FAIL -eq 0 ]]; then
    exit 0
else
    exit 1
fi
