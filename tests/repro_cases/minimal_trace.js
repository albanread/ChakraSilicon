// Minimal JIT tracing test
// This will trigger JIT compilation and show the generated assembly

print("Starting JIT trace test...");

// Test 1: Simple arithmetic function
function add(a, b) {
  return a + b;
}

// Test 2: Function that calls a built-in
function callArrayPush(arr, val) {
  return arr.push(val);
}

// Test 3: Math built-in
function mathSqrt(x) {
  return Math.sqrt(x);
}

// Test 4: Multiple operations
function complex(x, y) {
  var sum = x + y;
  var prod = x * y;
  return sum + prod;
}

print("\n=== Warming up functions (forcing JIT compilation) ===\n");

// Warm up add() - needs many iterations to trigger JIT
print("Warming up add()...");
var result1 = 0;
for (var i = 0; i < 10000; i++) {
  result1 = add(i, i + 1);
}
print("add() warmed up, last result: " + result1);

// Warm up callArrayPush()
print("\nWarming up callArrayPush()...");
var testArr = [];
for (var i = 0; i < 10000; i++) {
  callArrayPush(testArr, i);
}
print("callArrayPush() warmed up, array length: " + testArr.length);

// Warm up mathSqrt()
print("\nWarming up mathSqrt()...");
var result2 = 0;
for (var i = 0; i < 10000; i++) {
  result2 = mathSqrt(i);
}
print("mathSqrt() warmed up, last result: " + result2);

// Warm up complex()
print("\nWarming up complex()...");
var result3 = 0;
for (var i = 0; i < 10000; i++) {
  result3 = complex(i, i + 1);
}
print("complex() warmed up, last result: " + result3);

print("\n=== All functions warmed up ===");
print("If -TraceJitAsm is enabled, you should see disassembly above.");
print("Done.");
