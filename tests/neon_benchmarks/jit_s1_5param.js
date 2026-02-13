// jit_s1_5param.js — Scenario 1: 5 params (all in registers, control case)
// Expected: PASS on both interpreter and JIT
// Run: timeout 5 ch jit_s1_5param.js

function s5(a,b,c,d,e) { return a+b+c+d+e; }

// Single interpreted call
var r = s5(1,2,3,4,5);
print("S1_interp: " + (r === 15 ? "PASS" : "FAIL=" + r));

// JIT warmup
for (var i = 0; i < 300; i++) r = s5(1,2,3,4,5);
print("S1_postJIT: " + (r === 15 ? "PASS" : "FAIL=" + r));

print("DONE");
