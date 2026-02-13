// jit_7param_test.js — Isolated test for Bug B: 7+ parameter JS→JS calls
//
// Tests whether JIT-compiled calls with 7+ parameters correctly pass
// overflow arguments (those beyond the 6 that fit in x2-x7).
//
// On ARM64, the calling convention uses:
//   x0 = function, x1 = callInfo, x2-x7 = first 6 values (this + 5 args)
//   [CallerSP+0], [CallerSP+8], ... = overflow args (6th param onward)
//
// Bug B: When both caller and callee are JIT-compiled, the 7th+ parameters
// are read from wrong stack locations, causing crashes or wrong values.
//
// Run: timeout 5 ch jit_7param_test.js

// === Test 1: Exactly 7 params (first overflow) ===
function sum7(a, b, c, d, e, f, g) {
  return a + b + c + d + e + f + g;
}

// === Test 2: 8 params (two overflow slots) ===
function sum8(a, b, c, d, e, f, g, h) {
  return a + b + c + d + e + f + g + h;
}

// === Test 3: 6 params (no overflow — control case) ===
function sum6(a, b, c, d, e, f) {
  return a + b + c + d + e + f;
}

// === Test 4: 5 params (well within registers — control case) ===
function sum5(a, b, c, d, e) {
  return a + b + c + d + e;
}

// === Test 5: 7 params, check each param individually ===
function check7(a, b, c, d, e, f, g) {
  if (a !== 10) return "FAIL:a=" + a;
  if (b !== 20) return "FAIL:b=" + b;
  if (c !== 30) return "FAIL:c=" + c;
  if (d !== 40) return "FAIL:d=" + d;
  if (e !== 50) return "FAIL:e=" + e;
  if (f !== 60) return "FAIL:f=" + f;
  if (g !== 70) return "FAIL:g=" + g;
  return "OK";
}

// === Test 6: 9 params (three overflow slots) ===
function sum9(a, b, c, d, e, f, g, h, k) {
  return a + b + c + d + e + f + g + h + k;
}

// === Test 7: 7 params with string to detect function-object corruption ===
function checkStr7(a, b, c, d, e, f, g) {
  var s = "" + g;
  if (s.indexOf("function") === 0 || s.length > 50) {
    return "CORRUPTED:g=" + s.substring(0, 40);
  }
  return "OK:g=" + g;
}

var pass = 0;
var fail = 0;
var total = 0;

function run(label, fn) {
  total++;
  var ok = fn();
  if (ok) {
    pass++;
    print("  PASS: " + label);
  } else {
    fail++;
    print("  FAIL: " + label);
  }
}

print("=== 7+ Parameter JIT Tests ===");
print("");

// --- Control cases (should always work) ---
print("-- Control: 5 params (all in registers) --");
run("sum5 interpreted", function () {
  var r = sum5(1, 2, 3, 4, 5);
  if (r !== 15) { print("    got " + r + " expected 15"); return false; }
  return true;
});

print("-- Control: 6 params (all in registers) --");
run("sum6 interpreted", function () {
  var r = sum6(1, 2, 3, 4, 5, 6);
  if (r !== 21) { print("    got " + r + " expected 21"); return false; }
  return true;
});

// --- 7 param tests ---
print("");
print("-- 7 params: pre-JIT (interpreted) --");
run("sum7 single call", function () {
  var r = sum7(1, 2, 3, 4, 5, 6, 7);
  if (r !== 28) { print("    got " + r + " expected 28"); return false; }
  return true;
});

run("check7 single call", function () {
  var r = check7(10, 20, 30, 40, 50, 60, 70);
  if (r !== "OK") { print("    " + r); return false; }
  return true;
});

// --- JIT warmup for 6-param (control) ---
print("");
print("-- Warming up sum6 for JIT --");
for (var i = 0; i < 500; i++) {
  var r6 = sum6(1, 2, 3, 4, 5, 6);
}
run("sum6 post-JIT", function () {
  var r = sum6(1, 2, 3, 4, 5, 6);
  if (r !== 21) { print("    got " + r + " expected 21"); return false; }
  return true;
});

// --- JIT warmup for 5-param (control) ---
print("");
print("-- Warming up sum5 for JIT --");
for (var i = 0; i < 500; i++) {
  var r5 = sum5(1, 2, 3, 4, 5);
}
run("sum5 post-JIT", function () {
  var r = sum5(1, 2, 3, 4, 5);
  if (r !== 15) { print("    got " + r + " expected 15"); return false; }
  return true;
});

// --- JIT warmup for 7-param ---
print("");
print("-- Warming up sum7 for JIT (BUG B trigger) --");
var crashed = false;
for (var i = 0; i < 500; i++) {
  var r7 = sum7(1, 2, 3, 4, 5, 6, 7);
  if (r7 !== 28) {
    print("  FAIL at iter " + i + ": sum7 got " + r7 + " expected 28");
    crashed = true;
    fail++;
    total++;
    break;
  }
}
if (!crashed) {
  run("sum7 post-JIT warmup", function () {
    var r = sum7(1, 2, 3, 4, 5, 6, 7);
    if (r !== 28) { print("    got " + r + " expected 28"); return false; }
    return true;
  });
}

// --- JIT warmup for 8-param ---
print("");
print("-- Warming up sum8 for JIT --");
crashed = false;
for (var i = 0; i < 500; i++) {
  var r8 = sum8(1, 2, 3, 4, 5, 6, 7, 8);
  if (r8 !== 36) {
    print("  FAIL at iter " + i + ": sum8 got " + r8 + " expected 36");
    crashed = true;
    fail++;
    total++;
    break;
  }
}
if (!crashed) {
  run("sum8 post-JIT warmup", function () {
    var r = sum8(1, 2, 3, 4, 5, 6, 7, 8);
    if (r !== 36) { print("    got " + r + " expected 36"); return false; }
    return true;
  });
}

// --- JIT warmup for 9-param ---
print("");
print("-- Warming up sum9 for JIT --");
crashed = false;
for (var i = 0; i < 500; i++) {
  var r9 = sum9(1, 2, 3, 4, 5, 6, 7, 8, 9);
  if (r9 !== 45) {
    print("  FAIL at iter " + i + ": sum9 got " + r9 + " expected 45");
    crashed = true;
    fail++;
    total++;
    break;
  }
}
if (!crashed) {
  run("sum9 post-JIT warmup", function () {
    var r = sum9(1, 2, 3, 4, 5, 6, 7, 8, 9);
    if (r !== 45) { print("    got " + r + " expected 45"); return false; }
    return true;
  });
}

// --- check7 individual param verification post-JIT ---
print("");
print("-- Warming up check7 for JIT --");
crashed = false;
for (var i = 0; i < 500; i++) {
  var rc = check7(10, 20, 30, 40, 50, 60, 70);
  if (rc !== "OK") {
    print("  FAIL at iter " + i + ": check7 " + rc);
    crashed = true;
    fail++;
    total++;
    break;
  }
}
if (!crashed) {
  run("check7 post-JIT", function () {
    var r = check7(10, 20, 30, 40, 50, 60, 70);
    if (r !== "OK") { print("    " + r); return false; }
    return true;
  });
}

// --- String corruption check ---
print("");
print("-- Warming up checkStr7 for JIT (corruption detector) --");
crashed = false;
for (var i = 0; i < 500; i++) {
  var rs = checkStr7("a", "b", "c", "d", "e", "f", "label-" + i);
  if (rs.indexOf("CORRUPTED") === 0) {
    print("  FAIL at iter " + i + ": " + rs);
    crashed = true;
    fail++;
    total++;
    break;
  }
}
if (!crashed) {
  run("checkStr7 post-JIT", function () {
    var r = checkStr7("a", "b", "c", "d", "e", "f", "final");
    if (r !== "OK:g=final") { print("    got: " + r); return false; }
    return true;
  });
}

// === Summary ===
print("");
print("===========================");
print("  Passed: " + pass + " / " + total);
print("  Failed: " + fail + " / " + total);
print("===========================");
