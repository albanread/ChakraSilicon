#!/bin/bash

# Start the process in background
./build/chjita64_debug/bin/ch/ch examination/early_hang_test.js &
PID=$!

# Wait a moment for it to hang
sleep 2

# Attach with lldb and examine
lldb -p $PID <<LLDB_EOF
# Print backtrace
bt

# Print registers
register read

# Disassemble around PC
dis -c 20

# Try to continue
detach
quit
LLDB_EOF

# Kill the hung process
kill -9 $PID 2>/dev/null
