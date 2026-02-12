// test_fulljit_crash_diag.js
// Isolate exactly where FullJit exception crashes
// Uses fewer iterations to trigger SimpleJit first, then FullJit

function f() {
    throw 42;
}

// Phase 1: Interpreter (should work)
print("Phase 1: interpreter...");
for (var i = 0; i < 10; i++) {
    try { f(); } catch(e) {}
}
print("Phase 1: PASS");

// Phase 2: SimpleJit threshold (typically ~30 calls)
print("Phase 2: SimpleJit...");
for (var i = 0; i < 50; i++) {
    try { f(); } catch(e) {}
}
print("Phase 2: PASS");

// Phase 3: FullJit threshold (typically ~200+ calls)
print("Phase 3: approaching FullJit...");
for (var i = 0; i < 100; i++) {
    try { f(); } catch(e) {}
}
print("Phase 3a: 160 total, PASS");

for (var i = 0; i < 50; i++) {
    try { f(); } catch(e) {}
}
print("Phase 3b: 210 total, PASS");

for (var i = 0; i < 100; i++) {
    try { f(); } catch(e) {}
}
print("Phase 3c: 310 total, PASS");

for (var i = 0; i < 200; i++) {
    try { f(); } catch(e) {}
}
print("Phase 3d: 510 total, PASS");

print("ALL PHASES PASSED");
