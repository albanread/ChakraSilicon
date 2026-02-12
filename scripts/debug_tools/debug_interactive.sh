#!/bin/bash
lldb ./build/chjita64_debug/bin/ch/ch <<LLDB_EOF
# First, set breakpoint at main to let libraries load
b main
run examination/early_hang_test.js

# Now set breakpoints in the loaded library
b Js::FunctionBody::GenerateDynamicInterpreterThunk
b InterpreterThunkEmitter::EncodeInterpreterThunk

# Continue and see what we hit
c

# Print some info when we hit
bt
register read
c
LLDB_EOF
