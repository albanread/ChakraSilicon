// test_exception_minimal_fulljit.js
// Minimal test to trigger FullJit exception path
function f() {
    throw 42;
}

for (var i = 0; i < 500; i++) {
    try {
        f();
    } catch (e) {
        // caught
    }
}
print("PASS: caught 500 exceptions through FullJit");
