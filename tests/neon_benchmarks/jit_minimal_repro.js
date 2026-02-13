//-------------------------------------------------------------------------------------------------------
// jit_minimal_repro.js — Minimal reproducer for ARM64 JIT polymorphic TypedArray bugs
//
// Two confirmed bugs:
//
// BUG A: Double-polymorphism TypeError
//   When a shared harness function (that does `new ArrayType(n)`) calls a shared
//   callback (that does `c[i] = a[i] + b[i]`), and BOTH get JIT-compiled for one
//   TypedArray kind, switching to a second kind throws TypeError on the 2nd+ call
//   through the same harness.
//
// BUG B: 6-parameter argument slot corruption
//   A function with 6+ params that does TypedArray operations gets JIT-compiled,
//   and the 6th param silently becomes the function object itself (even without
//   any type change).
//
// Run: ch jit_minimal_repro.js
//-------------------------------------------------------------------------------------------------------

var pass = 0;
var fail = 0;

function test(label, fn) {
  try {
    fn();
    pass++;
    print("  PASS: " + label);
  } catch (e) {
    fail++;
    print("  FAIL: " + label + " -- " + e);
  }
}

// ===== BUG A: Double-polymorphism TypeError =====

print("=== BUG A: Double-polymorphism TypeError ===");

// Shared callback — does element access on typed arrays
function sharedAdd(a, b, c, n) {
  for (var i = 0; i < n; i++) c[i] = a[i] + b[i];
}

function sharedSub(a, b, c, n) {
  for (var i = 0; i < n; i++) c[i] = a[i] - b[i];
}

// Shared harness — creates typed arrays via constructor param AND calls callback
function harness(ArrayType, size, iters, opFn) {
  var a = new ArrayType(size);
  var b = new ArrayType(size);
  var c = new ArrayType(size);
  for (var i = 0; i < size; i++) {
    a[i] = i + 1;
    b[i] = i * 0.5;
  }
  for (var iter = 0; iter < iters; iter++) {
    opFn(a, b, c, size);
  }
  return c[0];
}

// Phase 1: warm up both harness + sharedAdd + sharedSub with Float32Array
// Use enough iterations to ensure JIT compilation of all three functions
test("Float32Array add warmup (harness + callback JIT)", function () {
  var r = harness(Float32Array, 50000, 100, sharedAdd);
  // a[0]=1, b[0]=0, so add => 1.0
  if (Math.abs(r - 1.0) > 0.01) throw new Error("got " + r);
});

test("Float32Array sub warmup (harness + callback JIT)", function () {
  var r = harness(Float32Array, 50000, 100, sharedSub);
  // a[0]=1, b[0]=0, so sub => 1.0
  if (Math.abs(r - 1.0) > 0.01) throw new Error("got " + r);
});

// Phase 2: call the SAME harness with Float64Array
// First call through harness with new type may trigger bailout + recompile
test("Float64Array add (first type change through harness)", function () {
  var r = harness(Float64Array, 100, 1, sharedAdd);
  if (Math.abs(r - 1.0) > 0.01) throw new Error("got " + r);
});

// BUG TRIGGER: second call through the recompiled harness with Float64Array
// The harness was recompiled, but sharedSub is still JIT-compiled for Float32Array
test("Float64Array sub (second call, BUG TRIGGER)", function () {
  var r = harness(Float64Array, 100, 1, sharedSub);
  if (Math.abs(r - 1.0) > 0.01) throw new Error("got " + r);
});

// Also test third type
test("Int32Array add (third type through harness)", function () {
  var r = harness(Int32Array, 100, 1, sharedAdd);
  if (r !== 1) throw new Error("got " + r);
});

print("");

// ===== BUG B: Argument slot corruption =====

print("=== BUG B: 6-param argument slot corruption ===");

function sixParams(name, ArrayType, size, fillVal, searchVal, label) {
  var arr = new ArrayType(size);
  arr.fill(fillVal);
  arr[size - 1] = searchVal;

  // Hot loop to ensure JIT
  var result = -1;
  for (var iter = 0; iter < 300; iter++) {
    result = arr.indexOf(searchVal);
  }

  // Check if label param got corrupted to the function object
  var labelStr = "" + label;
  if (labelStr.length > 100 || labelStr.indexOf("function") === 0) {
    throw new Error("param 6 corrupted to: " + labelStr.substring(0, 60));
  }
  if (result !== size - 1) {
    throw new Error("indexOf wrong: " + result);
  }
}

// Warm up with Int32Array — corruption can happen even without type change
test("6-param warmup + corruption check", function () {
  for (var w = 0; w < 100; w++) {
    sixParams("Int32Array", Int32Array, 1000, 0, -999, "iter-" + w);
  }
});

// After JIT, keep calling with same type
test("6-param post-JIT same type", function () {
  sixParams("Int32Array", Int32Array, 1000, 0, -999, "post-jit");
});

// Type change
test("6-param type change to Float32Array", function () {
  sixParams("Float32Array", Float32Array, 1000, 0, -999.5, "float32-test");
});

test("6-param type change to Uint8Array", function () {
  sixParams("Uint8Array", Uint8Array, 1000, 0, 255, "uint8-test");
});

print("");

// ===== Summary =====

print("===================================================");
print("  Passed: " + pass + " / " + (pass + fail));
print("  Failed: " + fail + " / " + (pass + fail));
print("===================================================");

if (fail > 0) {
  print("");
  print("  *** JIT BUG CONFIRMED ***");
  print("  These are pre-existing ARM64 JIT issues, not caused by NEON work.");
  print("  They affect any polymorphic TypedArray code and must be fixed");
  print("  before NEON acceleration can be safely layered on top.");
  print("");
  print("  Next step: trace with -Dump:BackEnd -Dump:GlobOpt -Dump:Lower");
  print("  to find where type guards / bailouts are missing.");
} else {
  print("  ALL PASSED (bugs not reproduced in this run)");
}

print("");
