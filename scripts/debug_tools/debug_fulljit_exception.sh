#!/bin/bash
# debug_fulljit_exception.sh — Capture crash info for FullJit exception handling
cd /Volumes/xc/chakrablue

# Approach: use CrashReporter to get the crash log, or atos to symbolicate.
# First just run and capture the crash report from /tmp/

# Run with LLDB and tell it to stop on SIGSEGV
lldb -b \
  -o "process handle SIGSEGV --stop true --pass false" \
  -o "process launch -- -ForceNative test_exception_minimal_fulljit.js" \
  -o "bt 30" \
  -o "register read pc sp fp lr x0 x1 x2 x3 x16 x17" \
  -o "disassemble -p -c 10" \
  -o "image lookup -a \$pc" \
  -o "quit" \
  -- ./build/chjita64_debug/bin/ch/ch \
  2>&1 | tee /tmp/fulljit_crash.log

echo ""
echo "=== Log saved to /tmp/fulljit_crash.log ==="
