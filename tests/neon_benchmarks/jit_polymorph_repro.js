//-------------------------------------------------------------------------------------------------------
// Copyright (C) ChakraSilicon Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// jit_polymorph_repro.js — Minimal reproducer for ARM64 JIT polymorphic TypedArray bug
//
// BUG: When a function that creates/accesses a TypedArray via a constructor parameter
// gets JIT-compiled after being called with one TypedArray kind (e.g. Float32Array),
// subsequent calls with a different kind (e.g. Float64Array) throw:
//
//   TypeError: Object doesn't support this action
//
// instead of bailing out and recompiling.  This affects ANY polymorphic TypedArray code,
// which is extremely common in real-world JS (audio processing, WebGL, data science
// libraries, generic utility functions, etc.).
//
// Run with: ch jit_polymorph_repro.js
//       or: ch -forcejit jit_polymorph_repro.js
//
//-------------------------------------------------------------------------------------------------------

var passed = 0;
var failed = 0;

function check(label, fn) {
  try {
    fn();
    passed++;
    print("  PASS: " + label);
  } catch (e) {
    failed++;
    print("  FAIL: " + label + " — " + e);
  }
}

// =====================================================================================
// Repro 1: Constructor polymorphism
//
// The simplest case.  A function receives a TypedArray constructor as a parameter,
// calls `new Ctor(n)`, and does element access.  After JIT profiles it for one type,
// calling with a second type throws.
// =====================================================================================

print("=== Repro 1: Constructor polymorphism ===");

function createAndFill(Ctor, n, value) {
  var arr = new Ctor(n);
  for (var i = 0; i < n; i++) {
    arr[i] = value;
  }
  return arr;
}

// Warm up with Float32Array to trigger JIT compilation
for (var w = 0; w < 500; w++) {
  createAndFill(Float32Array, 100, 1.5);
}

check("Float32Array after warmup", function () {
  var r = createAndFill(Float32Array, 10, 3.14);
  if (r.length !== 10) throw new Error("wrong length");
});

check("Float64Array (different type, same function)", function () {
  var r = createAndFill(Float64Array, 10, 3.14);
  if (r.length !== 10) throw new Error("wrong length");
});

check("Int32Array (third type)", function () {
  var r = createAndFill(Int32Array, 10, 42);
  if (r.length !== 10) throw new Error("wrong length");
});

check("Uint8Array (fourth type)", function () {
  var r = createAndFill(Uint8Array, 10, 200);
  if (r.length !== 10) throw new Error("wrong length");
});

print("");

// =====================================================================================
// Repro 2: Element-wise operation polymorphism
//
// A function that does arithmetic on pre-existing typed arrays.  This is the pattern
// that broke add_bench.js.
// =====================================================================================

print("=== Repro 2: Element-wise arithmetic on typed arrays ===");

function addArrays(a, b, c, n) {
  for (var i = 0; i < n; i++) {
    c[i] = a[i] + b[i];
  }
}

// Warm up with Float32Array
(function () {
  var a = new Float32Array(100);
  var b = new Float32Array(100);
  var c = new Float32Array(100);
  for (var i = 0; i < 100; i++) {
    a[i] = i;
    b[i] = i * 0.5;
  }
  for (var w = 0; w < 500; w++) {
    addArrays(a, b, c, 100);
  }
  check("addArrays with Float32Array (warmed)", function () {
    if (c[1] !== 1.5) throw new Error("wrong value: " + c[1]);
  });
})();

check("addArrays with Float64Array (cold type)", function () {
  var a = new Float64Array([1, 2, 3, 4]);
  var b = new Float64Array([10, 20, 30, 40]);
  var c = new Float64Array(4);
  addArrays(a, b, c, 4);
  if (c[0] !== 11) throw new Error("wrong value: " + c[0]);
});

check("addArrays with Int32Array (cold type)", function () {
  var a = new Int32Array([1, 2, 3, 4]);
  var b = new Int32Array([10, 20, 30, 40]);
  var c = new Int32Array(4);
  addArrays(a, b, c, 4);
  if (c[0] !== 11) throw new Error("wrong value: " + c[0]);
});

print("");

// =====================================================================================
// Repro 3: Argument slot corruption
//
// When a function with many parameters gets JIT-compiled and then called with a
// different TypedArray type, string parameters can resolve to the function object
// itself (printing its source code instead of the string).
// =====================================================================================

print("=== Repro 3: Argument slot corruption ===");

function processWithLabel(
  name,
  ArrayType,
  size,
  target,
  expectedIndex,
  scenario,
) {
  var arr = new ArrayType(size);
  for (var i = 0; i < size; i++) {
    arr[i] = i % 128;
  }
  if (expectedIndex >= 0 && expectedIndex < size) {
    arr[expectedIndex] = target;
  }

  // Hot loop to trigger JIT
  var result = -1;
  for (var iter = 0; iter < 200; iter++) {
    result = arr.indexOf(target);
  }

  // The bug: after JIT, `scenario` may resolve to the function reference
  // instead of the string that was passed.
  var scenarioStr = "" + scenario;
  var isCorrupted = scenarioStr.indexOf("function") === 0;

  return {
    result: result,
    expectedIndex: expectedIndex,
    scenario: scenarioStr,
    corrupted: isCorrupted,
  };
}

// Warm up with Int32Array
for (var w = 0; w < 50; w++) {
  processWithLabel("Int32Array", Int32Array, 1000, -99, 500, "warmup");
}

check("String param intact after JIT warmup (same type)", function () {
  var r = processWithLabel("Int32Array", Int32Array, 100, -99, 50, "test-same");
  if (r.corrupted) throw new Error("scenario corrupted to: " + r.scenario);
  if (r.result !== r.expectedIndex) throw new Error("wrong index");
});

check("String param intact with Float32Array (different type)", function () {
  var r = processWithLabel(
    "Float32Array",
    Float32Array,
    100,
    -99.5,
    50,
    "test-float32",
  );
  if (r.corrupted)
    throw new Error(
      "scenario corrupted to: " + r.scenario.substring(0, 60) + "...",
    );
  if (r.result !== r.expectedIndex) throw new Error("wrong index");
});

check("String param intact with Uint8Array (different type)", function () {
  var r = processWithLabel(
    "Uint8Array",
    Uint8Array,
    100,
    200,
    50,
    "test-uint8",
  );
  if (r.corrupted)
    throw new Error(
      "scenario corrupted to: " + r.scenario.substring(0, 60) + "...",
    );
  if (r.result !== r.expectedIndex) throw new Error("wrong index");
});

print("");

// =====================================================================================
// Repro 4: Real-world pattern — generic utility function
//
// This is how libraries like lodash or a data processing layer would write code.
// A single function handles any TypedArray.
// =====================================================================================

print("=== Repro 4: Real-world generic utility ===");

function typedArraySum(arr) {
  var sum = 0;
  for (var i = 0; i < arr.length; i++) {
    sum += arr[i];
  }
  return sum;
}

function typedArrayScale(arr, factor) {
  var result = new arr.constructor(arr.length);
  for (var i = 0; i < arr.length; i++) {
    result[i] = arr[i] * factor;
  }
  return result;
}

function typedArrayMap(src, dst, fn) {
  for (var i = 0; i < src.length; i++) {
    dst[i] = fn(src[i]);
  }
}

// Warm up with Float32Array
var warmArr = new Float32Array(200);
for (var i = 0; i < 200; i++) warmArr[i] = i;
for (var w = 0; w < 500; w++) typedArraySum(warmArr);
for (var w = 0; w < 500; w++) typedArrayScale(warmArr, 2.0);

check("typedArraySum — Float32Array (warmed)", function () {
  var r = typedArraySum(new Float32Array([1, 2, 3, 4, 5]));
  if (r !== 15) throw new Error("expected 15, got " + r);
});

check("typedArraySum — Float64Array", function () {
  var r = typedArraySum(new Float64Array([1, 2, 3, 4, 5]));
  if (r !== 15) throw new Error("expected 15, got " + r);
});

check("typedArraySum — Int32Array", function () {
  var r = typedArraySum(new Int32Array([1, 2, 3, 4, 5]));
  if (r !== 15) throw new Error("expected 15, got " + r);
});

check("typedArraySum — Int8Array", function () {
  var r = typedArraySum(new Int8Array([1, 2, 3, 4, 5]));
  if (r !== 15) throw new Error("expected 15, got " + r);
});

check("typedArrayScale — Float64Array", function () {
  var r = typedArrayScale(new Float64Array([1, 2, 3]), 10);
  if (r[2] !== 30) throw new Error("expected 30, got " + r[2]);
});

check("typedArrayScale — Int32Array", function () {
  var r = typedArrayScale(new Int32Array([1, 2, 3]), 10);
  if (r[2] !== 30) throw new Error("expected 30, got " + r[2]);
});

var sqrtFn = function (x) {
  return Math.sqrt(Math.abs(x));
};

check("typedArrayMap — Float64Array", function () {
  var src = new Float64Array([1, 4, 9, 16]);
  var dst = new Float64Array(4);
  typedArrayMap(src, dst, sqrtFn);
  if (dst[2] !== 3) throw new Error("expected 3, got " + dst[2]);
});

print("");

// =====================================================================================
// Repro 5: Mixed typed-array constructor in a loop
//
// Iterating over an array of constructors — a common pattern in test harnesses
// and configuration-driven code.
// =====================================================================================

print("=== Repro 5: Constructor array iteration ===");

var constructors = [
  { name: "Int8Array", ctor: Int8Array, val: 42 },
  { name: "Uint8Array", ctor: Uint8Array, val: 200 },
  { name: "Int16Array", ctor: Int16Array, val: 1234 },
  { name: "Uint16Array", ctor: Uint16Array, val: 1234 },
  { name: "Int32Array", ctor: Int32Array, val: 42 },
  { name: "Uint32Array", ctor: Uint32Array, val: 42 },
  { name: "Float32Array", ctor: Float32Array, val: 3.14 },
  { name: "Float64Array", ctor: Float64Array, val: 2.71828 },
];

function genericFillAndVerify(Ctor, size, value) {
  var arr = new Ctor(size);
  arr.fill(value);
  var expected = new Ctor([value])[0]; // round-trip
  for (var i = 0; i < size; i++) {
    if (expected !== expected) {
      // NaN
      if (arr[i] === arr[i]) return false;
    } else {
      if (arr[i] !== expected) return false;
    }
  }
  return true;
}

// Warm up with the first constructor
for (var w = 0; w < 500; w++) {
  genericFillAndVerify(constructors[0].ctor, 64, constructors[0].val);
}

// Now iterate all constructors through the same JIT-compiled function
for (var ci = 0; ci < constructors.length; ci++) {
  var entry = constructors[ci];
  check("genericFillAndVerify — " + entry.name, function () {
    var ok = genericFillAndVerify(entry.ctor, 64, entry.val);
    if (!ok) throw new Error("fill/verify mismatch");
  });
}

print("");

// =====================================================================================
// Repro 6: Shared hot callback — exact pattern from add_bench.js
//
// This is the EXACT pattern that triggered the bug: a shared loop function
// (addLoop) passed as a callback to a benchmark harness.  The callback gets
// JIT-compiled with type feedback from Float32Array element access, then
// the harness calls it again with Float64Array arrays.  The key factors:
//   - The callback is a SHARED top-level function (not per-call-site)
//   - It runs HOT (many iterations × large arrays) before type change
//   - It's invoked indirectly via a parameter (opFn)
// =====================================================================================

print("=== Repro 6: Shared hot callback (add_bench.js pattern) ===");

// Shared loop function — just like the original addLoop
function sharedAddLoop(a, b, c, n) {
  for (var i = 0; i < n; i++) {
    c[i] = a[i] + b[i];
  }
}

function sharedSubLoop(a, b, c, n) {
  for (var i = 0; i < n; i++) {
    c[i] = a[i] - b[i];
  }
}

// Harness that takes constructor + callback — just like benchBinaryOp
function runBench(name, ArrayType, size, iters, opFn) {
  var a = new ArrayType(size);
  var b = new ArrayType(size);
  var c = new ArrayType(size);
  for (var i = 0; i < size; i++) {
    a[i] = i * 0.5 + 1;
    b[i] = i * 0.25 + 0.5;
  }
  for (var iter = 0; iter < iters; iter++) {
    opFn(a, b, c, size);
  }
  // Verify first element by computing the expected value through opFn itself
  // (not hardcoded to addition — the old code always did a[0]+b[0] which was
  // wrong for subtraction and caused a false failure in Repro 6).
  var ea = new ArrayType([a[0]]);
  var eb = new ArrayType([b[0]]);
  var ec = new ArrayType(1);
  opFn(ea, eb, ec, 1);
  var expected = ec[0];
  if (Math.abs(c[0] - expected) > 0.01) {
    throw new Error("wrong result: " + c[0] + " expected " + expected);
  }
}

// Phase A: warm up sharedAddLoop with Float32Array — make it very hot
check(
  "Repro6 sharedAddLoop — Float32Array (warmup, 200 iters × 10000 elems)",
  function () {
    runBench("Float32Array", Float32Array, 10000, 200, sharedAddLoop);
  },
);

// Phase B: call with Float64Array through the SAME sharedAddLoop
check(
  "Repro6 sharedAddLoop — Float64Array (type change, same callback)",
  function () {
    runBench("Float64Array", Float64Array, 10000, 200, sharedAddLoop);
  },
);

// Phase C: call with Int32Array
check(
  "Repro6 sharedAddLoop — Int32Array (third type, same callback)",
  function () {
    runBench("Int32Array", Int32Array, 10000, 200, sharedAddLoop);
  },
);

// Also test sharedSubLoop with same pattern
check("Repro6 sharedSubLoop — Float32Array (warmup)", function () {
  runBench("Float32Array", Float32Array, 10000, 200, sharedSubLoop);
});

check("Repro6 sharedSubLoop — Float64Array (type change)", function () {
  runBench("Float64Array", Float64Array, 10000, 200, sharedSubLoop);
});

print("");

// =====================================================================================
// Repro 7: Shared callback with BOTH harness and callback reused
//
// In add_bench.js, BOTH benchBinaryOp and addLoop were shared.  The harness
// itself also got JIT-compiled for Float32Array's constructor.  This is the
// double-polymorphism case.
// =====================================================================================

print("=== Repro 7: Double polymorphism (harness + callback both shared) ===");

function sharedHarness(name, ArrayType, size, iters, opName, opFn) {
  var a = new ArrayType(size);
  var b = new ArrayType(size);
  var c = new ArrayType(size);
  for (var i = 0; i < size; i++) {
    a[i] = i * 0.5 + 1;
    b[i] = i * 0.25 + 0.5;
  }
  for (var iter = 0; iter < iters; iter++) {
    opFn(a, b, c, size);
  }
  // Quick correctness check
  var expected;
  switch (opName) {
    case "add":
      expected = a[0] + b[0];
      break;
    case "sub":
      expected = a[0] - b[0];
      break;
  }
  if (Math.abs(c[0] - expected) > 0.01) {
    throw new Error(
      name + " " + opName + ": wrong result " + c[0] + " expected " + expected,
    );
  }
}

function sharedAdd2(a, b, c, n) {
  for (var i = 0; i < n; i++) {
    c[i] = a[i] + b[i];
  }
}

function sharedSub2(a, b, c, n) {
  for (var i = 0; i < n; i++) {
    c[i] = a[i] - b[i];
  }
}

// Warm up BOTH sharedHarness and sharedAdd2/sharedSub2 with Float32Array
check("Repro7 Float32Array add (warmup harness+callback)", function () {
  sharedHarness("Float32Array", Float32Array, 50000, 100, "add", sharedAdd2);
});
check("Repro7 Float32Array sub (warmup harness+callback)", function () {
  sharedHarness("Float32Array", Float32Array, 50000, 100, "sub", sharedSub2);
});

// Now call the SAME sharedHarness + SAME sharedAdd2 with Float64Array
check(
  "Repro7 Float64Array add (type change in both harness+callback)",
  function () {
    sharedHarness("Float64Array", Float64Array, 50000, 100, "add", sharedAdd2);
  },
);
check("Repro7 Float64Array sub (type change)", function () {
  sharedHarness("Float64Array", Float64Array, 50000, 100, "sub", sharedSub2);
});

// And with Int32Array
check("Repro7 Int32Array add (third type)", function () {
  sharedHarness("Int32Array", Int32Array, 50000, 100, "add", sharedAdd2);
});

print("");

// =====================================================================================
// Repro 8: Argument count stress — 6+ params with type change
//
// The argument slot corruption in indexof_bench.js specifically happened with
// 6 parameters.  Test whether higher param counts exacerbate the issue.
// =====================================================================================

print("=== Repro 8: Many-parameter function with type change ===");

function sixParamFunc(name, ArrayType, size, fillVal, searchVal, label) {
  var arr = new ArrayType(size);
  arr.fill(fillVal);
  arr[size - 1] = searchVal;
  var result = -1;
  for (var iter = 0; iter < 300; iter++) {
    result = arr.indexOf(searchVal);
  }
  var labelStr = "" + label;
  if (labelStr.indexOf("function") === 0) {
    throw new Error(
      "label corrupted to function body: " + labelStr.substring(0, 50),
    );
  }
  if (result !== size - 1) {
    throw new Error("indexOf returned " + result + " expected " + (size - 1));
  }
}

// Warm with Int32Array — wrapped in check() because the corruption can
// trigger mid-warmup once the JIT kicks in and the label parameter gets
// corrupted on a subsequent iteration.
check("Repro8 Int32Array warmup (100 iters, 6 params)", function () {
  for (var w = 0; w < 100; w++) {
    sixParamFunc("Int32Array", Int32Array, 1000, 0, -999, "warmup-" + w);
  }
});

check("Repro8 Int32Array (warmed, 6 params)", function () {
  sixParamFunc("Int32Array", Int32Array, 1000, 0, -999, "test-int32");
});

check("Repro8 Float32Array (type change, 6 params)", function () {
  sixParamFunc("Float32Array", Float32Array, 1000, 0, -999.5, "test-float32");
});

check("Repro8 Uint8Array (type change, 6 params)", function () {
  sixParamFunc("Uint8Array", Uint8Array, 1000, 0, 255, "test-uint8");
});

check("Repro8 Float64Array (type change, 6 params)", function () {
  sixParamFunc("Float64Array", Float64Array, 1000, 0, -999.5, "test-float64");
});

check("Repro8 Int16Array (type change, 6 params)", function () {
  sixParamFunc("Int16Array", Int16Array, 1000, 0, -999, "test-int16");
});

print("");

// =====================================================================================
// Summary
// =====================================================================================

print("===================================================");
print("  JIT Polymorphic TypedArray Bug — Repro Results");
print("===================================================");
print("  Passed: " + passed + " / " + (passed + failed));
print("  Failed: " + failed + " / " + (passed + failed));
print("===================================================");

if (failed > 0) {
  print("");
  print("  *** BUG CONFIRMED ***");
  print("  The ARM64 JIT does not correctly handle polymorphic TypedArray");
  print(
    "  functions.  After JIT-compiling a function for one TypedArray kind,",
  );
  print("  calling it with a different kind throws TypeError instead of");
  print("  bailing out and recompiling.");
  print("");
  print("  Impact: HIGH — any generic TypedArray utility function will break");
  print("  after JIT compilation.  This affects real-world code patterns in:");
  print("    - Audio/DSP processing libraries");
  print("    - WebGL/canvas data manipulation");
  print("    - Scientific computing / data science");
  print("    - Generic utility libraries (lodash-style)");
  print("    - Test harnesses iterating over TypedArray types");
  print("");
  print("  Reproduction conditions (from add_bench.js analysis):");
  print("    - A SHARED function that does typed-array element access");
  print("      (e.g. c[i] = a[i] + b[i]) gets very hot with one type");
  print("    - The function is then called with arrays of a different type");
  print("    - Both the outer harness and the inner callback being shared");
  print("      (double-polymorphism) may increase likelihood");
  print("    - Functions with 6+ parameters may trigger argument slot");
  print("      corruption (string params resolve to function object)");
  print("");
  print("  Workaround: Use factory functions to create a fresh function");
  print("  instance per TypedArray kind, preventing type profile reuse.");
  print("");
  print("  Root cause is likely in the ARM64 JIT backend's type guard /");
  print("  bailout logic for TypedArray element access operations.");
  print("  The Lowerer or GlobOpt is specializing for one TypedArray kind");
  print("  without inserting proper type checks that would trigger a");
  print("  bailout + recompilation on type mismatch.  The argument slot");
  print("  corruption variant suggests a register allocation or frame");
  print("  layout issue in the ARM64 encoder when handling bailout from");
  print("  polymorphic call sites.");
} else {
  print("  STATUS: ALL PASSED (bug may be fixed or not reproduced)");
}

print("");
