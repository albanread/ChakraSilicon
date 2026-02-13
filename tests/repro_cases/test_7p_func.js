function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }

// Wrap in a function to avoid global-scope loop body JIT issues
function runTest() {
    for (var i = 0; i < 5000; i++) {
        var r = test7p(1, 2, 3, 4, 5, 6, 7);
        if (i % 500 === 0) print("iter " + i + ": " + r);
    }
    print("runTest DONE");
}
runTest();
