#!/bin/bash
# debug_fulljit_crash.sh — Get crash info without lldb
# Uses the macOS crash reporter and atos for symbolication
cd /Volumes/xc/chakrablue

# Clean old crash logs
rm -f /tmp/crash_*.log

# Run the crashing test with SIGBUS/SIGSEGV backtrace via libgmalloc
# Actually, use the simpler approach: just enable core dumps
ulimit -c unlimited

echo "Running test to crash..."
./build/chjita64_debug/bin/ch/ch -ForceNative test_exception_minimal_fulljit.js 2>/tmp/crash_stderr.log
EXIT=$?
echo "Exit code: $EXIT"

echo ""
echo "=== stderr output ==="
cat /tmp/crash_stderr.log | grep -E "XDATA|JITOutput|Finalize|ASSERTION|Segfault|signal" | head -20

echo ""
echo "=== Looking for crash report ==="
sleep 2
# macOS saves crash reports here
REPORT=$(ls -t ~/Library/Logs/DiagnosticReports/ch_*.ips 2>/dev/null | head -1)
if [ -z "$REPORT" ]; then
    REPORT=$(ls -t /Library/Logs/DiagnosticReports/ch_*.ips 2>/dev/null | head -1)
fi
if [ -n "$REPORT" ]; then
    echo "Found crash report: $REPORT"
    echo ""
    echo "=== Thread that crashed ==="
    # Extract the crashed thread backtrace
    python3 -c "
import json, sys
try:
    with open('$REPORT') as f:
        # Skip the header line
        lines = f.readlines()
        # Find the JSON start
        for i, line in enumerate(lines):
            if line.strip().startswith('{'):
                data = json.loads(''.join(lines[i:]))
                break
        else:
            data = json.loads(''.join(lines))
    
    # Get faulting thread
    ft = data.get('faultingThread', 0)
    threads = data.get('threads', [])
    if ft < len(threads):
        t = threads[ft]
        print(f'Faulting thread {ft}:')
        frames = t.get('frames', [])
        for j, frame in enumerate(frames[:20]):
            img = frame.get('imageIndex', -1)
            off = frame.get('imageOffset', 0)
            sym = frame.get('symbol', '')
            so = frame.get('symbolLocation', 0)
            print(f'  [{j}] offset={off:#x} sym={sym}+{so}')
    
    # exception info
    ex = data.get('exception', {})
    print(f'Exception type: {ex.get(\"type\",\"?\")} subtype: {ex.get(\"subtype\",\"?\")}')
    # Get the faulting address
    print(f'Signal: {ex.get(\"signal\",\"?\")}')
except Exception as e:
    print(f'Error parsing crash report: {e}')
    # Fallback: just grep for useful info
    import subprocess
    subprocess.run(['grep', '-E', 'Thread.*Crashed|Exception|fault|0x', '$REPORT'], capture_output=False)
" 2>&1
else
    echo "No crash report found. Trying atos..."
    echo ""
fi

echo ""
echo "=== Full XDATA output ==="
grep "XDATA" /tmp/crash_stderr.log
