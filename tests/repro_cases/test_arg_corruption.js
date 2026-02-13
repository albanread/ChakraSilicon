function test7p(a, b, c, d, e, f, g) {
    if (g !== 7 && g !== "7") { // "7" check in case of string conversion corruption
        throw new Error("Arg g corrupted: " + g);
    }
    return a + b + c + d + e + f + g;
}

print("Running 7-param test...");
try {
    for (var i = 0; i < 2000; i++) {
        var res = test7p(1, 2, 3, 4, 5, 6, 7);
        if (res !== 28) throw new Error("Result wrong: " + res);
    }
    print("PASS: 7-param test");
} catch (e) {
    print("FAIL: " + e);
}
