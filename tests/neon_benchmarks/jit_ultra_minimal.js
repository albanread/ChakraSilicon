// Ultra-minimal JIT tests - each scenario isolated
// Run: timeout 5 ch jit_ultra_minimal.js

// Scenario 1: 5 params (all in registers, should always work)
function s5(a,b,c,d,e) { return a+b+c+d+e; }
var r5 = 0;
for (var i = 0; i < 300; i++) r5 = s5(1,2,3,4,5);
print("S1_5param: " + (r5 === 15 ? "PASS" : "FAIL=" + r5));

// Scenario 2: 6 params (all in registers x2-x7, should work)
function s6(a,b,c,d,e,f) { return a+b+c+d+e+f; }
var r6 = 0;
for (var i = 0; i < 300; i++) r6 = s6(1,2,3,4,5,6);
print("S2_6param: " + (r6 === 21 ? "PASS" : "FAIL=" + r6));

// Scenario 3: 7 params (first overflow to stack)
function s7(a,b,c,d,e,f,g) { return a+b+c+d+e+f+g; }
var r7 = 0;
for (var i = 0; i < 300; i++) r7 = s7(1,2,3,4,5,6,7);
print("S3_7param: " + (r7 === 28 ? "PASS" : "FAIL=" + r7));

// Scenario 4: TypedArray.indexOf in hot loop (CallDirect varargs)
function s4() {
  var a = new Int32Array(4);
  a[0] = 42;
  var r = -1;
  for (var j = 0; j < 10; j++) r = a.indexOf(42);
  return r;
}
var r4 = -1;
for (var i = 0; i < 100; i++) r4 = s4();
print("S4_indexOf: " + (r4 === 0 ? "PASS" : "FAIL=" + r4));

// Scenario 5: TypedArray.fill in hot loop (CallDirect varargs)
function s5fill() {
  var a = new Int32Array(8);
  for (var j = 0; j < 10; j++) a.fill(99);
  return a[7];
}
var r5f = 0;
for (var i = 0; i < 100; i++) r5f = s5fill();
print("S5_fill: " + (r5f === 99 ? "PASS" : "FAIL=" + r5f));

// Scenario 6: TypedArray.includes (CallDirect varargs)
function s6inc() {
  var a = new Int32Array(4);
  a[2] = 77;
  var r = false;
  for (var j = 0; j < 10; j++) r = a.includes(77);
  return r;
}
var r6i = false;
for (var i = 0; i < 100; i++) r6i = s6inc();
print("S6_includes: " + (r6i === true ? "PASS" : "FAIL=" + r6i));

// Scenario 7: String.indexOf (CallDirect varargs, segfault risk)
function s7str() {
  var s = "hello world";
  var r = -1;
  for (var j = 0; j < 10; j++) r = s.indexOf("world");
  return r;
}
var r7s = -1;
for (var i = 0; i < 50; i++) r7s = s7str();
print("S7_strIndexOf: " + (r7s === 6 ? "PASS" : "FAIL=" + r7s));

// Scenario 8: 6-param function with TypedArray ops (corruption check)
function s8corrupt(a1,a2,a3,a4,a5,a6) {
  var arr = new Int32Array(4);
  arr[0] = 42;
  var r = -1;
  for (var j = 0; j < 10; j++) r = arr.indexOf(42);
  var s = "" + a6;
  if (s.indexOf("function") === 0) return "CORRUPTED";
  return "OK:" + a6;
}
var r8 = "";
for (var i = 0; i < 100; i++) r8 = s8corrupt("A","B","C","D","E","F");
print("S8_6p_corrupt: " + (r8 === "OK:F" ? "PASS" : "FAIL=" + r8));

print("DONE");
