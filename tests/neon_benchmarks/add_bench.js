//-------------------------------------------------------------------------------------------------------
// Copyright (C) ChakraSilicon Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// add_bench.js — Element-wise arithmetic loop & SIMD operation benchmarks (Phase 1)
//
// Measures performance of tight arithmetic loops over TypedArrays that would benefit
// from NEON vectorization (Phase 4 auto-vectorization) and exercises the SIMD operation
// backends that are now NEON-accelerated (Phase 1 SimdXxxOperation intrinsics).
//
// Run with: ch add_bench.js
//
//-------------------------------------------------------------------------------------------------------

var N = 1000000;
var ITERS = 100;

// =====================================================================================
// Section 1: Element-wise binary arithmetic loops on TypedArrays
//
// These tight loops are the primary targets for Phase 4 auto-vectorization.
// In the meantime they exercise the interpreter and JIT scalar paths.
// NEON fill (Phase 1) is used to initialize the arrays.
// =====================================================================================

// Factory that returns a fresh benchBinaryOp + loop functions.
// Each array-type section must get its own copy so that the JIT profiles
// the constructor and element accesses for one TypedArray kind only.
// Without this, the JIT compiles benchBinaryOp for Float32Array and then
// throws TypeError when called with Float64Array / Int32Array.
function makeBinaryBench() {
  function addLoop(a, b, c, n) {
    for (var i = 0; i < n; i++) {
      c[i] = a[i] + b[i];
    }
  }
  function subLoop(a, b, c, n) {
    for (var i = 0; i < n; i++) {
      c[i] = a[i] - b[i];
    }
  }
  function mulLoop(a, b, c, n) {
    for (var i = 0; i < n; i++) {
      c[i] = a[i] * b[i];
    }
  }

  var ops = { add: addLoop, sub: subLoop, mul: mulLoop };

  return function benchBinaryOp(name, ArrayType, size, iters, opName) {
    var opFn = ops[opName];
    var a = new ArrayType(size);
    var b = new ArrayType(size);
    var c = new ArrayType(size);

    for (var i = 0; i < size; i++) {
      a[i] = i * 0.5 + 1;
      b[i] = i * 0.25 + 0.5;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      opFn(a, b, c, size);
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    // Quick correctness check on first few elements
    var ok = true;
    opFn(a, b, c, size);
    for (var i = 0; i < Math.min(8, size); i++) {
      var expected;
      switch (opName) {
        case "add":
          expected = a[i] + b[i];
          break;
        case "sub":
          expected = a[i] - b[i];
          break;
        case "mul":
          expected = a[i] * b[i];
          break;
      }
      if (Math.abs(c[i] - expected) > 1e-4) {
        ok = false;
        break;
      }
    }

    var status = ok ? "OK" : "FAIL";
    print(
      name +
        " " +
        opName +
        " loop: " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s) [" +
        status +
        "]",
    );
  };
}

print("=== Element-wise Arithmetic Loop Benchmark ===");
print("Array size: " + N + " elements, Iterations: " + ITERS);
print("");

// Each section gets a fresh benchmark function so the JIT doesn't
// carry stale type profiles across different TypedArray kinds.
(function () {
  var bench = makeBinaryBench();
  print("--- Float32Array ---");
  bench("Float32Array", Float32Array, N, ITERS, "add");
  bench("Float32Array", Float32Array, N, ITERS, "sub");
  bench("Float32Array", Float32Array, N, ITERS, "mul");
  print("");
})();

(function () {
  var bench = makeBinaryBench();
  print("--- Float64Array ---");
  bench("Float64Array", Float64Array, N, ITERS, "add");
  bench("Float64Array", Float64Array, N, ITERS, "sub");
  bench("Float64Array", Float64Array, N, ITERS, "mul");
  print("");
})();

(function () {
  var bench = makeBinaryBench();
  print("--- Int32Array ---");
  bench("Int32Array", Int32Array, N, ITERS, "add");
  bench("Int32Array", Int32Array, N, ITERS, "sub");
  bench("Int32Array", Int32Array, N, ITERS, "mul");
  print("");
})();

// =====================================================================================
// Section 2: Unary operation loops
// =====================================================================================

function makeUnaryBench() {
  function negLoop(a, b, n) {
    for (var i = 0; i < n; i++) {
      b[i] = -a[i];
    }
  }
  function absLoop(a, b, n) {
    for (var i = 0; i < n; i++) {
      b[i] = Math.abs(a[i]);
    }
  }

  var ops = { neg: negLoop, abs: absLoop };

  return function benchUnaryOp(name, ArrayType, size, iters, opName) {
    var opFn = ops[opName];
    var a = new ArrayType(size);
    var b = new ArrayType(size);

    for (var i = 0; i < size; i++) {
      a[i] = (i % 200) - 100 + 0.5;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      opFn(a, b, size);
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    print(
      name +
        " " +
        opName +
        " loop: " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s)",
    );
  };
}

print("--- Unary Operations ---");
(function () {
  var bench = makeUnaryBench();
  bench("Float32Array", Float32Array, N, ITERS, "neg");
  bench("Float32Array", Float32Array, N, ITERS, "abs");
})();
(function () {
  var bench = makeUnaryBench();
  bench("Float64Array", Float64Array, N, ITERS, "neg");
  bench("Float64Array", Float64Array, N, ITERS, "abs");
})();
(function () {
  var bench = makeUnaryBench();
  bench("Int32Array", Int32Array, N, ITERS, "neg");
})();
print("");

// =====================================================================================
// Section 3: Scalar broadcast operations (c[i] = a[i] * k)
// =====================================================================================

function makeBenchScalarBroadcast() {
  return function benchScalarBroadcast(name, ArrayType, size, iters, scalar) {
    var a = new ArrayType(size);
    var c = new ArrayType(size);

    for (var i = 0; i < size; i++) {
      a[i] = i * 0.1 + 1;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      for (var i = 0; i < size; i++) {
        c[i] = a[i] * scalar;
      }
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    var ok = true;
    for (var i = 0; i < Math.min(8, size); i++) {
      if (Math.abs(c[i] - a[i] * scalar) > 1e-4) {
        ok = false;
        break;
      }
    }
    var status = ok ? "OK" : "FAIL";
    print(
      name +
        " scale by " +
        scalar +
        ": " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s) [" +
        status +
        "]",
    );
  };
}

print("--- Scalar Broadcast (c[i] = a[i] * k) ---");
(function () {
  var bench = makeBenchScalarBroadcast();
  bench("Float32Array", Float32Array, N, ITERS, 2.5);
})();
(function () {
  var bench = makeBenchScalarBroadcast();
  bench("Float64Array", Float64Array, N, ITERS, 2.5);
})();
(function () {
  var bench = makeBenchScalarBroadcast();
  bench("Int32Array", Int32Array, N, ITERS, 3);
})();
print("");

// =====================================================================================
// Section 4: Reduction operations (sum, min, max, dot product)
// =====================================================================================

function makeBenchReduction() {
  function sumReduce(a, n) {
    var sum = 0;
    for (var i = 0; i < n; i++) {
      sum += a[i];
    }
    return sum;
  }

  function minReduce(a, n) {
    var m = a[0];
    for (var i = 1; i < n; i++) {
      if (a[i] < m) m = a[i];
    }
    return m;
  }

  function maxReduce(a, n) {
    var m = a[0];
    for (var i = 1; i < n; i++) {
      if (a[i] > m) m = a[i];
    }
    return m;
  }

  function dotProduct(a, n) {
    var sum = 0;
    for (var i = 0; i < n; i++) {
      sum += a[i] * a[i];
    }
    return sum;
  }

  var ops = { sum: sumReduce, min: minReduce, max: maxReduce, dot: dotProduct };

  return function benchReduction(name, ArrayType, size, iters, opName) {
    var opFn = ops[opName];
    var a = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      a[i] = ((i % 100) - 50) * 0.01;
    }

    var start = Date.now();
    var result = 0;
    for (var iter = 0; iter < iters; iter++) {
      result = opFn(a, size);
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    print(
      name +
        " " +
        opName +
        ": " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s) result=" +
        result.toFixed(6),
    );
  };
}

print("--- Reductions ---");
(function () {
  var bench = makeBenchReduction();
  bench("Float32Array", Float32Array, N, ITERS, "sum");
  bench("Float32Array", Float32Array, N, ITERS, "min");
  bench("Float32Array", Float32Array, N, ITERS, "max");
  bench("Float32Array", Float32Array, N, ITERS, "dot");
})();
(function () {
  var bench = makeBenchReduction();
  bench("Float64Array", Float64Array, N, ITERS, "sum");
  bench("Float64Array", Float64Array, N, ITERS, "dot");
})();
(function () {
  var bench = makeBenchReduction();
  bench("Int32Array", Int32Array, N, ITERS, "sum");
  bench("Int32Array", Int32Array, N, ITERS, "min");
  bench("Int32Array", Int32Array, N, ITERS, "max");
})();
print("");

// =====================================================================================
// Section 5: Math.min / Math.max on TypedArrays (exercises FindMinOrMax NEON path)
// =====================================================================================

// Use smaller arrays for Math.min/max.apply since it has argument count limits
var MM_SIZE = 50000;
var MM_ITERS = 200;

print("--- Math.min / Math.max on Arrays (FindMinOrMax path) ---");
print("Array size: " + MM_SIZE + " elements, Iterations: " + MM_ITERS);

// Use manual loop-based min/max instead of apply (which has stack limits)
function makeBenchLoopMinMax() {
  return function benchLoopMinMax(name, ArrayType, size, iters, findMax) {
    var a = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      a[i] = ((i * 7 + 13) % size) - size / 2;
    }
    var extremePos = ((size * 3) / 4) | 0;
    if (findMax) {
      a[extremePos] = 999999;
    } else {
      a[extremePos] = -999999;
    }

    var funcName = findMax ? "max" : "min";
    var result;

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      result = a[0];
      for (var i = 1; i < size; i++) {
        if (findMax) {
          if (a[i] > result) result = a[i];
        } else {
          if (a[i] < result) result = a[i];
        }
      }
    }
    var elapsed = Date.now() - start;
    var megaElementsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    var expected = findMax ? 999999 : -999999;
    var status = result === expected ? "OK" : "FAIL(got " + result + ")";
    print(
      name +
        " loop-" +
        funcName +
        ": " +
        elapsed +
        "ms (" +
        megaElementsPerSec.toFixed(1) +
        " M elem/s) [" +
        status +
        "]",
    );
  };
}

(function () {
  var bench = makeBenchLoopMinMax();
  bench("Float32Array", Float32Array, N, ITERS, false);
  bench("Float32Array", Float32Array, N, ITERS, true);
})();
(function () {
  var bench = makeBenchLoopMinMax();
  bench("Float64Array", Float64Array, N, ITERS, false);
  bench("Float64Array", Float64Array, N, ITERS, true);
})();
(function () {
  var bench = makeBenchLoopMinMax();
  bench("Int32Array", Int32Array, N, ITERS, false);
  bench("Int32Array", Int32Array, N, ITERS, true);
})();
print("");

// =====================================================================================
// Section 6: Fused multiply-add pattern (c[i] = a[i] * b[i] + c[i])
// =====================================================================================

function makeBenchFMA() {
  return function benchFMA(name, ArrayType, size, iters) {
    var a = new ArrayType(size);
    var b = new ArrayType(size);
    var c = new ArrayType(size);

    for (var i = 0; i < size; i++) {
      a[i] = i * 0.001 + 1;
      b[i] = i * 0.0005 + 0.5;
      c[i] = 0;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      for (var i = 0; i < size; i++) {
        c[i] = a[i] * b[i] + c[i];
      }
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    print(
      name +
        " FMA: " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s)",
    );
  };
}

print("--- Fused Multiply-Add Pattern ---");
(function () {
  var bench = makeBenchFMA();
  bench("Float32Array", Float32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchFMA();
  bench("Float64Array", Float64Array, N, ITERS);
})();
print("");

// =====================================================================================
// Section 7: Array copy benchmark (exercises NEON copy helpers)
// =====================================================================================

function makeBenchCopy() {
  return function benchCopy(name, ArrayType, size, iters) {
    var src = new ArrayType(size);
    var dst = new ArrayType(size);

    for (var i = 0; i < size; i++) {
      src[i] = i * 0.5;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      dst.set(src);
    }
    var elapsed = Date.now() - start;
    var megaElementsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    var ok = true;
    for (var i = 0; i < Math.min(16, size); i++) {
      if (dst[i] !== src[i]) {
        ok = false;
        break;
      }
    }
    var status = ok ? "OK" : "FAIL";
    print(
      name +
        " .set() copy: " +
        elapsed +
        "ms (" +
        megaElementsPerSec.toFixed(1) +
        " M elem/s) [" +
        status +
        "]",
    );
  };
}

print("--- TypedArray.set() Copy ---");
(function () {
  var bench = makeBenchCopy();
  bench("Float32Array", Float32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchCopy();
  bench("Float64Array", Float64Array, N, ITERS);
})();
(function () {
  var bench = makeBenchCopy();
  bench("Int32Array", Int32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchCopy();
  bench("Int16Array", Int16Array, N, ITERS);
})();
(function () {
  var bench = makeBenchCopy();
  bench("Int8Array", Int8Array, N, ITERS);
})();
print("");

// =====================================================================================
// Section 8: Array reverse benchmark (exercises NEON reverse path)
// =====================================================================================

function makeBenchReverse() {
  return function benchReverse(name, ArrayType, size, iters) {
    var arr = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      arr[i] = i;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      arr.reverse();
    }
    var elapsed = Date.now() - start;
    var megaElementsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    var ok = true;
    if (iters % 2 === 0) {
      for (var i = 0; i < Math.min(16, size); i++) {
        if (arr[i] !== i) {
          ok = false;
          break;
        }
      }
    } else {
      for (var i = 0; i < Math.min(16, size); i++) {
        if (arr[i] !== size - 1 - i) {
          ok = false;
          break;
        }
      }
    }
    var status = ok ? "OK" : "FAIL";
    print(
      name +
        " .reverse(): " +
        elapsed +
        "ms (" +
        megaElementsPerSec.toFixed(1) +
        " M elem/s) [" +
        status +
        "]",
    );
  };
}

print("--- TypedArray.reverse() ---");
(function () {
  var bench = makeBenchReverse();
  bench("Float32Array", Float32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchReverse();
  bench("Float64Array", Float64Array, N, ITERS);
})();
(function () {
  var bench = makeBenchReverse();
  bench("Int32Array", Int32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchReverse();
  bench("Int16Array", Int16Array, N, ITERS);
})();
(function () {
  var bench = makeBenchReverse();
  bench("Int8Array", Int8Array, N, ITERS);
})();
print("");

// =====================================================================================
// Section 9: Mixed-size stress test
// =====================================================================================

print("--- Size Sweep: add loop across array lengths ---");
var SWEEP_SIZES = [
  1, 3, 4, 5, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 100, 255, 256, 257,
  1000, 4096, 10000, 100000, 1000000,
];
var SWEEP_ITERS_BASE = 100000000;

function benchSizeSweep(ArrayType, typeName) {
  for (var s = 0; s < SWEEP_SIZES.length; s++) {
    var size = SWEEP_SIZES[s];
    var iters = Math.max(1, Math.min(100000, (SWEEP_ITERS_BASE / size) | 0));

    var a = new ArrayType(size);
    var b = new ArrayType(size);
    var c = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      a[i] = i;
      b[i] = i * 0.5;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      for (var i = 0; i < size; i++) {
        c[i] = a[i] + b[i];
      }
    }
    var elapsed = Date.now() - start;
    if (elapsed === 0) elapsed = 1;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;
    var nsPerElem = (elapsed * 1e6) / (size * iters);

    print(
      "  " +
        typeName +
        "[" +
        size +
        "] x " +
        iters +
        ": " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s, " +
        nsPerElem.toFixed(2) +
        " ns/elem)",
    );
  }
}

benchSizeSweep(Float32Array, "Float32Array");
print("");

// =====================================================================================
// Section 10: Conditional store pattern (for future Phase 4 masked vectorization)
// =====================================================================================

function makeBenchConditionalStore() {
  return function benchConditionalStore(name, ArrayType, size, iters) {
    var a = new ArrayType(size);
    var b = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      a[i] = ((i % 200) - 100) * 0.1;
    }

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      for (var i = 0; i < size; i++) {
        if (a[i] > 0) {
          b[i] = a[i];
        } else {
          b[i] = 0;
        }
      }
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    print(
      name +
        " conditional store: " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s)",
    );
  };
}

print("--- Conditional Store (ReLU pattern) ---");
(function () {
  var bench = makeBenchConditionalStore();
  bench("Float32Array", Float32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchConditionalStore();
  bench("Float64Array", Float64Array, N, ITERS);
})();
(function () {
  var bench = makeBenchConditionalStore();
  bench("Int32Array", Int32Array, N, ITERS);
})();
print("");

// =====================================================================================
// Section 11: Clamp pattern (for future Phase 4)
// =====================================================================================

function makeBenchClamp() {
  return function benchClamp(name, ArrayType, size, iters) {
    var a = new ArrayType(size);
    var c = new ArrayType(size);
    for (var i = 0; i < size; i++) {
      a[i] = ((i % 300) - 150) * 0.01;
    }

    var lo = -0.5;
    var hi = 0.5;

    var start = Date.now();
    for (var iter = 0; iter < iters; iter++) {
      for (var i = 0; i < size; i++) {
        c[i] = a[i] < lo ? lo : a[i] > hi ? hi : a[i];
      }
    }
    var elapsed = Date.now() - start;
    var megaOpsPerSec = (size * iters) / (elapsed / 1000) / 1e6;

    print(
      name +
        " clamp [" +
        lo +
        ", " +
        hi +
        "]: " +
        elapsed +
        "ms (" +
        megaOpsPerSec.toFixed(1) +
        " M ops/s)",
    );
  };
}

print("--- Clamp Pattern ---");
(function () {
  var bench = makeBenchClamp();
  bench("Float32Array", Float32Array, N, ITERS);
})();
(function () {
  var bench = makeBenchClamp();
  bench("Float64Array", Float64Array, N, ITERS);
})();
print("");

print("=== Benchmark Complete ===");
