//-------------------------------------------------------------------------------------------------------
// Copyright (C) ChakraSilicon Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// fill_bench.js — TypedArray.fill() benchmark for NEON acceleration (Phase 1)
//
// Measures fill performance across all TypedArray element types and sizes.
// Run with: ch fill_bench.js
//
//-------------------------------------------------------------------------------------------------------

var N = 1000000;
var ITERS = 200;

function benchFill(name, ArrayType, fillValue) {
  var arr = new ArrayType(N);
  var start = Date.now();
  for (var iter = 0; iter < ITERS; iter++) {
    arr.fill(fillValue);
  }
  var elapsed = Date.now() - start;
  var megaElementsPerSec = (N * ITERS) / (elapsed / 1000) / 1e6;
  print(
    name +
      ".fill(" +
      fillValue +
      "): " +
      elapsed +
      "ms (" +
      megaElementsPerSec.toFixed(1) +
      " M elem/s)",
  );

  // Quick correctness check
  // For float types the stored value may differ from the JS double (e.g. Math.fround(3.14) !== 3.14).
  // NaN !== NaN by definition, so we must use isNaN() for that case.
  var expected = new ArrayType([fillValue])[0]; // round-trip through the TypedArray
  var ok = true;
  for (var i = 0; i < 16; i++) {
    if (expected !== expected) {
      // NaN case: both must be NaN
      if (arr[i] === arr[i]) {
        ok = false;
        break;
      }
    } else {
      if (arr[i] !== expected) {
        ok = false;
        break;
      }
    }
  }
  if (!ok) {
    print("  ERROR: fill correctness check failed!");
  }
}

function benchFillZero(name, ArrayType) {
  var arr = new ArrayType(N);
  // Pre-fill with non-zero to ensure fill(0) actually writes
  for (var i = 0; i < N; i++) arr[i] = 1;
  var start = Date.now();
  for (var iter = 0; iter < ITERS; iter++) {
    arr.fill(0);
  }
  var elapsed = Date.now() - start;
  var megaElementsPerSec = (N * ITERS) / (elapsed / 1000) / 1e6;
  print(
    name +
      ".fill(0): " +
      elapsed +
      "ms (" +
      megaElementsPerSec.toFixed(1) +
      " M elem/s)",
  );
}

print("=== TypedArray.fill() Benchmark ===");
print("Array size: " + N + " elements, Iterations: " + ITERS);
print("");

// Zero fills (memset path)
print("--- Zero Fill (memset path) ---");
benchFillZero("Float32Array", Float32Array);
benchFillZero("Float64Array", Float64Array);
benchFillZero("Int32Array", Int32Array);
benchFillZero("Int16Array", Int16Array);
benchFillZero("Int8Array", Int8Array);
benchFillZero("Uint32Array", Uint32Array);
benchFillZero("Uint16Array", Uint16Array);
benchFillZero("Uint8Array", Uint8Array);
print("");

// Non-zero fills (NEON path when available)
print("--- Non-Zero Fill (NEON-accelerated path) ---");
benchFill("Float32Array", Float32Array, 3.14);
benchFill("Float64Array", Float64Array, 2.71828);
benchFill("Int32Array", Int32Array, 42);
benchFill("Int16Array", Int16Array, 1234);
benchFill("Int8Array", Int8Array, 77);
benchFill("Uint32Array", Uint32Array, 42);
benchFill("Uint16Array", Uint16Array, 1234);
benchFill("Uint8Array", Uint8Array, 200);
print("");

// Partial fill benchmark
print("--- Partial Fill (middle of array) ---");
function benchPartialFill(name, ArrayType, fillValue) {
  var arr = new ArrayType(N);
  var quarter = (N / 4) | 0;
  var half = (N / 2) | 0;
  var start = Date.now();
  for (var iter = 0; iter < ITERS; iter++) {
    arr.fill(fillValue, quarter, quarter + half);
  }
  var elapsed = Date.now() - start;
  var megaElementsPerSec = (half * ITERS) / (elapsed / 1000) / 1e6;
  print(
    name +
      ".fill(" +
      fillValue +
      ", " +
      quarter +
      ", " +
      (quarter + half) +
      "): " +
      elapsed +
      "ms (" +
      megaElementsPerSec.toFixed(1) +
      " M elem/s)",
  );
}

benchPartialFill("Float32Array", Float32Array, 1.5);
benchPartialFill("Int32Array", Int32Array, 99);
print("");

// Small array fills (overhead-sensitive)
print("--- Small Array Fill (N=16, many iterations) ---");
var SMALL_ITERS = 1000000;
function benchSmallFill(name, ArrayType, size, fillValue) {
  var arr = new ArrayType(size);
  var start = Date.now();
  for (var iter = 0; iter < SMALL_ITERS; iter++) {
    arr.fill(fillValue);
  }
  var elapsed = Date.now() - start;
  print(
    name +
      "[" +
      size +
      "].fill(" +
      fillValue +
      "): " +
      elapsed +
      "ms (" +
      SMALL_ITERS +
      " iters)",
  );
}

benchSmallFill("Float32Array", Float32Array, 16, 1.0);
benchSmallFill("Int32Array", Int32Array, 16, 7);
benchSmallFill("Int8Array", Int8Array, 16, 3);
benchSmallFill("Float32Array", Float32Array, 4, 1.0);
benchSmallFill("Float32Array", Float32Array, 1, 1.0);
print("");

// Edge case: fill with special values
print("--- Special Value Fills ---");
benchFill("Float32Array(NaN)", Float32Array, NaN);
benchFill("Float32Array(Inf)", Float32Array, Infinity);
benchFill("Float32Array(-Inf)", Float32Array, -Infinity);
benchFill("Float64Array(NaN)", Float64Array, NaN);

print("");
print("=== Benchmark Complete ===");
