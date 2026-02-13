// jit_bugb_only.js — Minimal isolated repro for Bug B: 6-param argument slot corruption
//
// The 6th parameter of a function that does TypedArray ops silently becomes
// the function object itself after JIT compilation — even with a single type.
//
// Run:
//   ch jit_bugb_only.js
//   ch -Dump:BackEnd jit_bugb_only.js 2>&1 | tee bugb_dump.txt
//   ch -Dump:Lower  jit_bugb_only.js 2>&1 | tee bugb_lower.txt
//   ch -Dump:GlobOpt jit_bugb_only.js 2>&1 | tee bugb_globopt.txt

function sixParams(a1, a2, a3, a4, a5, a6) {
  var arr = new Int32Array(64);
  arr.fill(0);
  arr[63] = -999;

  var result = -1;
  for (var iter = 0; iter < 200; iter++) {
    result = arr.indexOf(-999);
  }

  // Check if a6 got corrupted to the function object
  var s = "" + a6;
  if (s.length > 50 || s.indexOf("function") === 0) {
    return "CORRUPTED:" + s.substring(0, 40);
  }
  return "OK:result=" + result + ",a6=" + a6;
}

// Warm up — JIT should kick in during this loop
for (var i = 0; i < 80; i++) {
  var r = sixParams("A", "B", "C", "D", "E", "label-" + i);
  if (r.indexOf("CORRUPTED") === 0) {
    print("FAIL at warmup iter " + i + ": " + r);
    break;
  }
}

// Post-JIT calls
for (var j = 0; j < 5; j++) {
  var r = sixParams("A", "B", "C", "D", "E", "post-" + j);
  if (r.indexOf("CORRUPTED") === 0) {
    print("FAIL at post-JIT iter " + j + ": " + r);
  } else {
    print("PASS post-JIT iter " + j + ": " + r);
  }
}
