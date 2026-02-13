//-------------------------------------------------------------------------------------------------------
// Copyright (C) ChakraSilicon Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// indexof_bench.js — TypedArray.indexOf() benchmark for NEON acceleration (Phase 1)
//
// Measures indexOf performance across TypedArray element types.
// Tests worst-case (last element), best-case (first element), and not-found scenarios.
// Run with: ch indexof_bench.js
//
//-------------------------------------------------------------------------------------------------------

var N = 1000000;
var ITERS = 200;

// Factory that returns a fresh benchIndexOf function.
// Each array-type section must get its own copy so that the JIT profiles
// the element accesses for one TypedArray kind only.  Without this, the
// JIT compiles benchIndexOf for the first ArrayType it sees and then
// corrupts argument slots (printing the function body instead of the
// scenario string) when called with a different typed-array kind.
function makeBenchIndexOf() {
  return function benchIndexOf(label, ArrayType, size, target, expectedIndex) {
    var arr = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      arr[i] = i % 128;
    }
    if (expectedIndex >= 0 && expectedIndex < size) {
      arr[expectedIndex] = target;
    }

    var start = Date.now();
    var result = -1;
    for (var iter = 0; iter < ITERS; iter++) {
      result = arr.indexOf(target);
    }
    var elapsed = Date.now() - start;
    var megaScansPerSec = (size * ITERS) / (elapsed / 1000) / 1e6;

    var status = result === expectedIndex ? "OK" : "FAIL(got " + result + ")";
    print(
      label +
        ": " +
        elapsed +
        "ms (" +
        megaScansPerSec.toFixed(1) +
        " M elem/s) [" +
        status +
        "]",
    );
  };
}

function makeBenchIncludes() {
  return function benchIncludes(
    label,
    ArrayType,
    size,
    target,
    expectedResult,
  ) {
    var arr = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      arr[i] = i % 128;
    }
    if (expectedResult) {
      arr[size - 1] = target;
    }

    var start = Date.now();
    var result = false;
    for (var iter = 0; iter < ITERS; iter++) {
      result = arr.includes(target);
    }
    var elapsed = Date.now() - start;
    var megaScansPerSec = (size * ITERS) / (elapsed / 1000) / 1e6;

    var status = result === expectedResult ? "OK" : "FAIL(got " + result + ")";
    print(
      label +
        ": " +
        elapsed +
        "ms (" +
        megaScansPerSec.toFixed(1) +
        " M elem/s) [" +
        status +
        "]",
    );
  };
}

print("=== TypedArray.indexOf() / .includes() Benchmark ===");
print("Array size: " + N + " elements, Iterations: " + ITERS);
print("");

// --- Worst case: target at the very end ---
print("--- Worst Case: Target at Last Element ---");
(function () {
  var bench = makeBenchIndexOf();
  bench("Int32Array.indexOf [last]", Int32Array, N, -99, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Uint32Array.indexOf [last]", Uint32Array, N, 9999, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Float32Array.indexOf [last]", Float32Array, N, -99.5, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Float64Array.indexOf [last]", Float64Array, N, -99.5, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Int16Array.indexOf [last]", Int16Array, N, -99, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Int8Array.indexOf [last]", Int8Array, N, -99, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Uint8Array.indexOf [last]", Uint8Array, N, 200, N - 1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Uint16Array.indexOf [last]", Uint16Array, N, 9999, N - 1);
})();
print("");

// --- Best case: target at the first element ---
print("--- Best Case: Target at First Element ---");
var BEST_ITERS = 5000;

function makeBenchIndexOfBest(bestIters) {
  return function benchIndexOfBest(label, ArrayType, size, target) {
    var arr = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      arr[i] = i + 1;
    }
    arr[0] = target;

    var start = Date.now();
    var result = -1;
    for (var iter = 0; iter < bestIters; iter++) {
      result = arr.indexOf(target);
    }
    var elapsed = Date.now() - start;

    var status = result === 0 ? "OK" : "FAIL(got " + result + ")";
    print(
      label + ": " + elapsed + "ms (" + bestIters + " iters) [" + status + "]",
    );
  };
}

(function () {
  var bench = makeBenchIndexOfBest(BEST_ITERS);
  bench("Int32Array.indexOf [first]", Int32Array, N, -42);
})();
(function () {
  var bench = makeBenchIndexOfBest(BEST_ITERS);
  bench("Float32Array.indexOf [first]", Float32Array, N, -42.5);
})();
(function () {
  var bench = makeBenchIndexOfBest(BEST_ITERS);
  bench("Int8Array.indexOf [first]", Int8Array, N, -42);
})();
print("");

// --- Not found: full scan with no match ---
print("--- Not Found: Full Scan ---");
(function () {
  var bench = makeBenchIndexOf();
  bench("Int32Array.indexOf [not-found]", Int32Array, N, -99999, -1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Float32Array.indexOf [not-found]", Float32Array, N, -99999.5, -1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Float64Array.indexOf [not-found]", Float64Array, N, -99999.5, -1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Int8Array.indexOf [not-found]", Int8Array, N, -99, -1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Uint8Array.indexOf [not-found]", Uint8Array, N, 255, -1);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Int16Array.indexOf [not-found]", Int16Array, N, -9999, -1);
})();
print("");

// --- Mid-array: target at 50% position ---
print("--- Mid-Array: Target at 50% ---");
var MID = (N / 2) | 0;
(function () {
  var bench = makeBenchIndexOf();
  bench("Int32Array.indexOf [mid]", Int32Array, N, -99, MID);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Float32Array.indexOf [mid]", Float32Array, N, -99.5, MID);
})();
(function () {
  var bench = makeBenchIndexOf();
  bench("Int8Array.indexOf [mid]", Int8Array, N, -99, MID);
})();
print("");

// --- includes() benchmark ---
print("--- .includes() ---");
(function () {
  var bench = makeBenchIncludes();
  bench("Int32Array.includes [found-last]", Int32Array, N, -99, true);
})();
(function () {
  var bench = makeBenchIncludes();
  bench("Int32Array.includes [not-found]", Int32Array, N, -99999, false);
})();
(function () {
  var bench = makeBenchIncludes();
  bench("Float32Array.includes [found-last]", Float32Array, N, -99.5, true);
})();
(function () {
  var bench = makeBenchIncludes();
  bench("Float32Array.includes [not-found]", Float32Array, N, -99999.5, false);
})();
(function () {
  var bench = makeBenchIncludes();
  bench("Int8Array.includes [found-last]", Int8Array, N, -99, true);
})();
(function () {
  var bench = makeBenchIncludes();
  bench("Uint8Array.includes [found-last]", Uint8Array, N, 200, true);
})();
print("");

// --- Small array indexOf (overhead-sensitive) ---
print("--- Small Array indexOf (N=16, many iterations) ---");
var SMALL_N = 16;
var SMALL_ITERS_IDX = 1000000;

function makeBenchSmallIndexOf(smallIters) {
  return function benchSmallIndexOf(
    label,
    ArrayType,
    size,
    target,
    expectedIndex,
  ) {
    var arr = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      arr[i] = i;
    }
    if (expectedIndex >= 0) {
      arr[expectedIndex] = target;
    }

    var start = Date.now();
    var result = -1;
    for (var iter = 0; iter < smallIters; iter++) {
      result = arr.indexOf(target);
    }
    var elapsed = Date.now() - start;

    var status = result === expectedIndex ? "OK" : "FAIL(got " + result + ")";
    print(
      label + ": " + elapsed + "ms (" + smallIters + " iters) [" + status + "]",
    );
  };
}

(function () {
  var bench = makeBenchSmallIndexOf(SMALL_ITERS_IDX);
  bench("Int32Array[16].indexOf(15) [pos 15]", Int32Array, SMALL_N, 15, 15);
  bench("Int32Array[16].indexOf(99) [not-found]", Int32Array, SMALL_N, 99, -1);
})();
(function () {
  var bench = makeBenchSmallIndexOf(SMALL_ITERS_IDX);
  bench("Float32Array[16].indexOf(15) [pos 15]", Float32Array, SMALL_N, 15, 15);
})();
(function () {
  var bench = makeBenchSmallIndexOf(SMALL_ITERS_IDX);
  bench("Int8Array[16].indexOf(15) [pos 15]", Int8Array, SMALL_N, 15, 15);
})();
(function () {
  var bench = makeBenchSmallIndexOf(SMALL_ITERS_IDX);
  bench("Uint8Array[16].indexOf(15) [pos 15]", Uint8Array, SMALL_N, 15, 15);
})();
print("");

// --- Edge cases ---
print("--- Edge Cases ---");
(function benchEdgeCases() {
  // NaN: indexOf should return -1, includes should return true (for Float arrays)
  var farr = new Float32Array(1000);
  for (var i = 0; i < 1000; i++) farr[i] = i;
  farr[500] = NaN;

  var idxResult = farr.indexOf(NaN);
  var inclResult = farr.includes(NaN);
  print(
    "Float32Array.indexOf(NaN) = " +
      idxResult +
      " (expected: -1) [" +
      (idxResult === -1 ? "OK" : "FAIL") +
      "]",
  );
  print(
    "Float32Array.includes(NaN) = " +
      inclResult +
      " (expected: true) [" +
      (inclResult === true ? "OK" : "FAIL") +
      "]",
  );

  // -0 vs +0
  var zarr = new Float64Array(1000);
  for (var i = 0; i < 1000; i++) zarr[i] = i + 1;
  zarr[0] = -0;

  var zeroIdx = zarr.indexOf(0);
  var negZeroIdx = zarr.indexOf(-0);
  print(
    "Float64Array with -0 at [0]: indexOf(0) = " +
      zeroIdx +
      ", indexOf(-0) = " +
      negZeroIdx +
      " [" +
      (zeroIdx === 0 && negZeroIdx === 0 ? "OK" : "FAIL") +
      "]",
  );

  // Empty array
  var empty = new Int32Array(0);
  var emptyResult = empty.indexOf(42);
  print(
    "Int32Array(0).indexOf(42) = " +
      emptyResult +
      " (expected: -1) [" +
      (emptyResult === -1 ? "OK" : "FAIL") +
      "]",
  );

  // Single element
  var single = new Int32Array(1);
  single[0] = 42;
  var singleFound = single.indexOf(42);
  var singleNotFound = single.indexOf(99);
  print(
    "Int32Array(1).indexOf(42) = " +
      singleFound +
      " (expected: 0) [" +
      (singleFound === 0 ? "OK" : "FAIL") +
      "]",
  );
  print(
    "Int32Array(1).indexOf(99) = " +
      singleNotFound +
      " (expected: -1) [" +
      (singleNotFound === -1 ? "OK" : "FAIL") +
      "]",
  );

  // Length = 3 (not a multiple of NEON vector width)
  var three = new Int32Array([10, 20, 30]);
  print(
    "Int32Array([10,20,30]).indexOf(30) = " +
      three.indexOf(30) +
      " (expected: 2) [" +
      (three.indexOf(30) === 2 ? "OK" : "FAIL") +
      "]",
  );
  print(
    "Int32Array([10,20,30]).indexOf(99) = " +
      three.indexOf(99) +
      " (expected: -1) [" +
      (three.indexOf(99) === -1 ? "OK" : "FAIL") +
      "]",
  );

  // Length = 5 (4 + 1 tail element)
  var five = new Float32Array([1.0, 2.0, 3.0, 4.0, 5.0]);
  print(
    "Float32Array([1..5]).indexOf(5.0) = " +
      five.indexOf(5.0) +
      " (expected: 4) [" +
      (five.indexOf(5.0) === 4 ? "OK" : "FAIL") +
      "]",
  );
})();
print("");

print("=== Benchmark Complete ===");
