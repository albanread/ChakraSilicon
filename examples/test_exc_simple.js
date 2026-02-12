// Simple try/catch JIT test - minimal case
function testTryCatch() {
    try {
        throw "hello";
    } catch(e) {
        return e;
    }
}

// Warmup to trigger JIT
for (var i = 0; i < 200; i++) {
    var r = testTryCatch();
    if (r !== "hello") {
        print("FAIL at iteration " + i + ": " + r);
        break;
    }
}
print("PASS: try/catch survived " + i + " iterations including JIT");
