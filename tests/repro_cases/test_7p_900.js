function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }
var N = 900;
for (var i = 0; i < N; i++) {
    var r = test7p(1, 2, 3, 4, 5, 6, 7);
    if (i >= N - 10 || i % 100 === 0) print("iter " + i + ": " + r);
}
print("DONE");
