// Test: Is it the CALLER or CALLEE JIT that crashes?
// Separate the two functions so we can control which gets JIT'd

function test7pCallee(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }

// Warm up the callee to JIT it
for (var w = 0; w < 2000; w++) {
    test7pCallee(1, 2, 3, 4, 5, 6, 7);
}
print("Callee warmed up (JIT'd). Single call from interpreter:");
var r = test7pCallee(1, 2, 3, 4, 5, 6, 7);
print("Result: " + r + " (expected 28)");

// Now call from a new function that will be JIT'd
function caller() {
    for (var i = 0; i < 2000; i++) {
        var r = test7pCallee(1, 2, 3, 4, 5, 6, 7);
        if (i % 200 === 0) print("caller iter " + i + ": " + r);
        if (r !== 28) { print("FAIL at " + i); return; }
    }
    print("caller PASSED");
}
caller();
