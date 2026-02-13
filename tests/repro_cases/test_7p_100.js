function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }
for (var i = 0; i < 100; i++) {
    var r = test7p(1, 2, 3, 4, 5, 6, 7);
    print("iter " + i + ": " + r);
}
