//-------------------------------------------------------------------------------------------------------
// Test Case: JIT→INT Call Interface Examination
//
// Purpose: Trigger JIT-compiled code calling interpreter-mode built-in functions
//          so we can examine how the x64 (working) implementation handles this transition.
//
// Strategy:
//   1. Run a loop to force JIT compilation of the outer function
//   2. Within JIT code, call various built-in functions that run in interpreter mode
//   3. Use different types of built-ins to understand parameter passing
//
// Usage:
//   ./dist/chjitx64/ch examination/test_jit_to_int.js        # x64 (working, under Rosetta)
//   ./dist/chjita64/ch examination/test_jit_to_int.js        # ARM64 (broken)
//
// Debug with lldb:
//   lldb ./dist/chjitx64/ch
//   (lldb) break set -n InterpreterStackFrame::InterpreterHelper
//   (lldb) run examination/test_jit_to_int.js
//-------------------------------------------------------------------------------------------------------

print("=== JIT→INT Call Interface Test ===\n");

//-------------------------------------------------------------------------------------------------------
// Test 1: Simple built-in with fixed arguments
//-------------------------------------------------------------------------------------------------------
print("Test 1: Math.max with fixed arguments");

function testMathMax() {
    // Math.max is typically a built-in that runs in interpreter mode
    var result = Math.max(42, 17);
    return result;
}

// Warm up to trigger JIT compilation
for (var i = 0; i < 10000; i++) {
    testMathMax();
}

// Final execution (should be JIT→INT)
var result1 = testMathMax();
print("  Result: " + result1 + " (expected: 42)");
print("  Status: " + (result1 === 42 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 2: Built-in with variable arguments
//-------------------------------------------------------------------------------------------------------
print("Test 2: Math.max with variable arguments");

function testMathMaxVar(a, b, c) {
    // Call with different argument counts to test varargs handling
    return Math.max(a, b, c);
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testMathMaxVar(1, 2, 3);
}

var result2 = testMathMaxVar(100, 50, 75);
print("  Result: " + result2 + " (expected: 100)");
print("  Status: " + (result2 === 100 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 3: String built-ins (different parameter types)
//-------------------------------------------------------------------------------------------------------
print("Test 3: String.fromCharCode");

function testStringFromCharCode() {
    // String built-ins often run in interpreter mode
    return String.fromCharCode(65, 66, 67);
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testStringFromCharCode();
}

var result3 = testStringFromCharCode();
print("  Result: '" + result3 + "' (expected: 'ABC')");
print("  Status: " + (result3 === "ABC" ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 4: Array built-ins with objects
//-------------------------------------------------------------------------------------------------------
print("Test 4: Array.isArray");

function testArrayIsArray(obj) {
    return Array.isArray(obj);
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testArrayIsArray([1, 2, 3]);
}

var result4a = testArrayIsArray([1, 2, 3]);
var result4b = testArrayIsArray({a: 1});
print("  Array test: " + result4a + " (expected: true)");
print("  Object test: " + result4b + " (expected: false)");
print("  Status: " + (result4a === true && result4b === false ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 5: Multiple built-in calls in sequence
//-------------------------------------------------------------------------------------------------------
print("Test 5: Multiple built-in calls in sequence");

function testMultipleCalls(x, y) {
    var a = Math.max(x, y);
    var b = Math.min(x, y);
    var c = Math.abs(x - y);
    return a + b + c;
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testMultipleCalls(10, 5);
}

var result5 = testMultipleCalls(10, 5);
print("  Result: " + result5 + " (expected: 20, calculation: max(10,5)=10 + min(10,5)=5 + abs(10-5)=5)");
print("  Status: " + (result5 === 20 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 6: Built-in with this context
//-------------------------------------------------------------------------------------------------------
print("Test 6: Built-in with 'this' context");

function testWithContext() {
    var str = "hello";
    return str.toUpperCase();
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testWithContext();
}

var result6 = testWithContext();
print("  Result: '" + result6 + "' (expected: 'HELLO')");
print("  Status: " + (result6 === "HELLO" ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 7: Built-in returning different types
//-------------------------------------------------------------------------------------------------------
print("Test 7: Built-in returning different types");

function testDifferentReturnTypes() {
    var num = parseInt("42");
    var bool = isNaN(NaN);
    var str = String(123);
    return [num, bool, str];
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testDifferentReturnTypes();
}

var result7 = testDifferentReturnTypes();
print("  parseInt('42'): " + result7[0] + " (expected: 42)");
print("  isNaN(NaN): " + result7[1] + " (expected: true)");
print("  String(123): '" + result7[2] + "' (expected: '123')");
var pass7 = (result7[0] === 42 && result7[1] === true && result7[2] === "123");
print("  Status: " + (pass7 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 8: Nested JIT→INT calls
//-------------------------------------------------------------------------------------------------------
print("Test 8: Nested JIT→INT calls");

function innerJitFunc(x) {
    return Math.sqrt(x);
}

function outerJitFunc(a, b) {
    // JIT function calling another JIT function that calls built-in
    var sum = a + b;
    return innerJitFunc(sum);
}

// Warm up both functions
for (var i = 0; i < 10000; i++) {
    innerJitFunc(16);
}
for (var i = 0; i < 10000; i++) {
    outerJitFunc(9, 7);
}

var result8 = outerJitFunc(9, 7);
print("  Result: " + result8 + " (expected: 4, sqrt(9+7)=sqrt(16)=4)");
print("  Status: " + (result8 === 4 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 9: Varargs stress test
//-------------------------------------------------------------------------------------------------------
print("Test 9: Varargs stress test");

function testVarargsStress() {
    // Call built-in with many arguments to stress parameter passing
    return Math.max(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testVarargsStress();
}

var result9 = testVarargsStress();
print("  Result: " + result9 + " (expected: 10)");
print("  Status: " + (result9 === 10 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Test 10: Object method built-ins
//-------------------------------------------------------------------------------------------------------
print("Test 10: Object method built-ins");

function testObjectMethods() {
    var obj = {a: 1, b: 2, c: 3};
    var keys = Object.keys(obj);
    return keys.length;
}

// Warm up
for (var i = 0; i < 10000; i++) {
    testObjectMethods();
}

var result10 = testObjectMethods();
print("  Result: " + result10 + " (expected: 3)");
print("  Status: " + (result10 === 3 ? "PASS" : "FAIL"));
print("");

//-------------------------------------------------------------------------------------------------------
// Summary
//-------------------------------------------------------------------------------------------------------
print("=== Test Summary ===");
print("All tests completed.");
print("");
print("To debug with lldb:");
print("  1. lldb ./dist/chjitx64/ch");
print("  2. break set -n InterpreterStackFrame::InterpreterHelper");
print("  3. break set -r '.*InterpreterThunk.*'");
print("  4. run examination/test_jit_to_int.js");
print("  5. When breakpoint hits, examine:");
print("     - registers: register read x0 x1 x2 x3 x4 x5 x6 x7  (ARM64)");
print("     - registers: register read rdi rsi rdx rcx r8 r9     (x64)");
print("     - stack: memory read -f x -c 16 $sp");
print("     - disassembly: disassemble -b");
print("");
print("To compare with ARM64:");
print("  ./dist/chjita64/ch examination/test_jit_to_int.js");
print("  (This will likely hang or crash on Test 1)");
