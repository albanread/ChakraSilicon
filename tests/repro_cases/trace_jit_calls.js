//-------------------------------------------------------------------------------------------------------
// JIT Caller Tracing Test
//
// This script is designed to trigger JIT compilation and trace the CALLER code -
// the JIT-generated machine code that calls INTO the interpreter thunks.
//
// What we want to see:
//   1. How the JIT-compiled function sets up arguments (registers vs stack)
//   2. What the call instruction looks like (call/bl/blr and to what address)
//   3. Stack layout before the call
//   4. Register state (X0-X7 on ARM64, RDI/RSI/RDX/RCX/R8/R9 on x64)
//
// Usage:
//   export CHAKRA_TRACE_JIT_ASM=1
//   ./ch examination/trace_jit_calls.js 2>&1 | tee jit_trace.log
//
// On ARM64, look for:
//   - How arguments are loaded into X0-X7
//   - The 'bl' or 'blr' instruction that calls the thunk
//   - Any 'stp' instructions that save state to stack before the call
//
// On x64, look for:
//   - How arguments are loaded into RDI, RSI, RDX, RCX, R8, R9
//   - The 'call' instruction that calls the thunk
//   - Any 'mov' or 'push' instructions that set up the stack
//-------------------------------------------------------------------------------------------------------

print("=== JIT Caller Tracing Test ===\n");
print("This test will warm up functions to force JIT compilation.");
print("The tracer will dump the JIT-generated machine code.\n");

//-------------------------------------------------------------------------------------------------------
// Test 1: Simple single built-in call
//-------------------------------------------------------------------------------------------------------
print("\n--- Test 1: Single Array.push call ---");

function callArrayPush(arr, value) {
  // Single built-in call - simplest case
  // Expected: JIT will generate code to:
  //   1. Load 'arr' into first arg register
  //   2. Load 'push' method
  //   3. Load 'value' into second arg register
  //   4. Call the interpreter thunk
  return arr.push(value);
}

// Warm up - run enough times to trigger JIT compilation
var arr1 = [];
print("Warming up callArrayPush...");
for (var i = 0; i < 150; i++) {
  callArrayPush(arr1, i);
}
print("callArrayPush warmed. Array length: " + arr1.length);

//-------------------------------------------------------------------------------------------------------
// Test 2: Math built-in with numeric argument
//-------------------------------------------------------------------------------------------------------
print("\n--- Test 2: Single Math.sqrt call ---");

function callMathSqrt(x) {
  // Math.sqrt with a single numeric argument
  // Expected: JIT will generate code to:
  //   1. Load 'x' into arg register (might be in FP register)
  //   2. Call Math.sqrt interpreter thunk
  return Math.sqrt(x);
}

print("Warming up callMathSqrt...");
var sum = 0;
for (var i = 0; i < 150; i++) {
  sum += callMathSqrt(i);
}
print("callMathSqrt warmed. Sum: " + sum);

//-------------------------------------------------------------------------------------------------------
// Test 3: Multiple arguments to built-in
//-------------------------------------------------------------------------------------------------------
print("\n--- Test 3: Multiple arguments (String.slice) ---");

function callStringSlice(str, start, end) {
  // String.slice with 3 total arguments (this + 2 params)
  // Expected: JIT will generate code to:
  //   1. Load 'str' into first arg register (this)
  //   2. Load 'start' into second arg register
  //   3. Load 'end' into third arg register
  //   4. Call String.prototype.slice interpreter thunk
  return str.slice(start, end);
}

print("Warming up callStringSlice...");
var testStr = "Hello, ChakraCore JIT!";
var slices = "";
for (var i = 0; i < 150; i++) {
  slices = callStringSlice(testStr, i % 10, (i % 10) + 5);
}
print("callStringSlice warmed. Last slice: " + slices);

//-------------------------------------------------------------------------------------------------------
// Test 4: Two consecutive built-in calls
//-------------------------------------------------------------------------------------------------------
print("\n--- Test 4: Two consecutive calls ---");

function callTwoBuiltins(arr, x) {
  // Two built-in calls in sequence
  // Expected: Two separate call sequences:
  //   First call to Math.sqrt
  //   Then call to Array.push
  var sqrt = Math.sqrt(x);
  arr.push(sqrt);
  return sqrt;
}

print("Warming up callTwoBuiltins...");
var arr2 = [];
for (var i = 0; i < 150; i++) {
  callTwoBuiltins(arr2, i);
}
print("callTwoBuiltins warmed. Array length: " + arr2.length);

//-------------------------------------------------------------------------------------------------------
// Test 5: Very simple case - just return a built-in call
//-------------------------------------------------------------------------------------------------------
print("\n--- Test 5: Minimal function (just return built-in result) ---");

function minimal(x) {
  // Absolutely minimal case - just call and return
  return Math.abs(x);
}

print("Warming up minimal...");
var absSum = 0;
for (var i = -75; i < 75; i++) {
  absSum += minimal(i);
}
print("minimal warmed. Sum of absolutes: " + absSum);

//-------------------------------------------------------------------------------------------------------
// Summary
//-------------------------------------------------------------------------------------------------------
print("\n=== All functions warmed and JIT-compiled ===");
print("\nCheck the trace output above for:");
print("  1. Function prologues (stack frame setup)");
print("  2. Argument loading sequence (mov/ldr instructions)");
print("  3. Call instructions (call/bl/blr)");
print(
  "  4. The target address of calls (should be interpreter thunk addresses)",
);
print("\nFor ARM64 specifically:");
print("  - Look for X0 = function object, X1 = callInfo, X2-X7 = args");
print("  - Look for 'bl <address>' or 'blr <register>' to thunk");
print("  - Check if there are 'stp' instructions saving args to stack");
print("\nFor x64 specifically:");
print("  - Look for RDI = function, RSI = callInfo, RDX/RCX/R8/R9 = args");
print("  - Look for 'call <address>' or 'call <register>' to thunk");
print("  - Check for stack manipulation before call");
