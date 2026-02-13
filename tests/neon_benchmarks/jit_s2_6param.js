// jit_s2_6param.js — Scenario 2: 6 params (first overflow slot on ARM64)
//
// On ARM64, the JIT calling convention uses:
//   x0 = function, x1 = callInfo, x2-x7 = first 6 values (this + 5 args)
// So a function with 6 named params has 8 total slots (function + callInfo + this + 6 args).
// Slot 8 (the 6th named param 'f') is the first to overflow to the stack at [CallerSP+0].
//
// Bug B: The DarwinPCS shadow store writes the function object to [SP+0],
// which is the same location as the first overflow arg, clobbering param 'f'.
//
// Expected on interpreter: PASS (all results correct)
// Expected on JIT (Bug B present): FAIL — param 'f' becomes the function object
//
// Run: timeout 5 ch jit_s2_6param.js

// --- Test A: Sum of 6 params ---
function sum6(a, b, c, d, e, f) {
  return a + b + c + d + e + f;
}

// Single interpreted call (before JIT)
var r = sum6(1, 2, 3, 4, 5, 6);
print("S2A_interp: " + (r === 21 ? "PASS" : "FAIL=" + r));

// JIT warmup loop
for (var i = 0; i < 500; i++) r = sum6(1, 2, 3, 4, 5, 6);
print("S2A_postJIT: " + (r === 21 ? "PASS" : "FAIL=" + r));

// --- Test B: Check each param individually ---
function check6(a, b, c, d, e, f) {
  if (a !== 10) return "FAIL:a=" + a;
  if (b !== 20) return "FAIL:b=" + b;
  if (c !== 30) return "FAIL:c=" + c;
  if (d !== 40) return "FAIL:d=" + d;
  if (e !== 50) return "FAIL:e=" + e;
  if (f !== 60) return "FAIL:f=" + f;
  return "OK";
}

// Single interpreted call
var rc = check6(10, 20, 30, 40, 50, 60);
print("S2B_interp: " + (rc === "OK" ? "PASS" : rc));

// JIT warmup loop
for (var i = 0; i < 500; i++) rc = check6(10, 20, 30, 40, 50, 60);
print("S2B_postJIT: " + (rc === "OK" ? "PASS" : rc));

// --- Test C: String corruption detection ---
// If param 'f' becomes the function object, stringifying it will contain "function"
function checkStr6(a, b, c, d, e, f) {
  var s = "" + f;
  if (s.indexOf("function") === 0 || s.length > 50) {
    return "CORRUPTED:f=" + s.substring(0, 60);
  }
  return "OK:f=" + f;
}

// Single interpreted call
var rs = checkStr6("A", "B", "C", "D", "E", "hello");
print("S2C_interp: " + (rs === "OK:f=hello" ? "PASS" : rs));

// JIT warmup
var corrupted = false;
for (var i = 0; i < 500; i++) {
  rs = checkStr6("A", "B", "C", "D", "E", "val-" + i);
  if (rs.indexOf("CORRUPTED") === 0) {
    print("S2C_JIT_FAIL at iter " + i + ": " + rs);
    corrupted = true;
    break;
  }
}
if (!corrupted) {
  rs = checkStr6("A", "B", "C", "D", "E", "final");
  print("S2C_postJIT: " + (rs === "OK:f=final" ? "PASS" : rs));
}

// --- Test D: Return only the 6th param (isolation) ---
function return6th(a, b, c, d, e, f) {
  return f;
}

var rf = return6th(1, 2, 3, 4, 5, 999);
print("S2D_interp: " + (rf === 999 ? "PASS" : "FAIL=" + rf));

for (var i = 0; i < 500; i++) rf = return6th(1, 2, 3, 4, 5, 999);
print("S2D_postJIT: " + (rf === 999 ? "PASS" : "FAIL=" + rf));

// --- Test E: 6 params with typeof check on f ---
function typeof6th(a, b, c, d, e, f) {
  return typeof f;
}

var rt = typeof6th(1, 2, 3, 4, 5, 6);
print("S2E_interp: " + (rt === "number" ? "PASS" : "FAIL:typeof=" + rt));

for (var i = 0; i < 500; i++) rt = typeof6th(1, 2, 3, 4, 5, 6);
print("S2E_postJIT: " + (rt === "number" ? "PASS" : "FAIL:typeof=" + rt));

print("DONE");
