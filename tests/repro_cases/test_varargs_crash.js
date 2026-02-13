function testVarargs() {
    var arr = new Int32Array(4);
    arr[0] = 42;
    var r = -1;
    for (var j = 0; j < 10; j++) {
        r = arr.indexOf(42);
    }
    return r;
}

print("Running varargs test...");
try {
    for (var i = 0; i < 200; i++) {
        var r = testVarargs();
        if (r !== 0) {
            throw new Error("Result wrong: " + r);
        }
    }
    print("PASS: Varargs test");
} catch (e) {
    print("FAIL: " + e);
}
