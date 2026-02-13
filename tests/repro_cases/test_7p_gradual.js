// Test: Gradually increase iterations to find JIT tier-up crash point
function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }

for (var i = 0; i < 2000; i++) {
    try {
        var r = test7p(1, 2, 3, 4, 5, 6, 7);
        if (i % 100 === 0) print("iter " + i + ": r=" + r);
        if (r !== 28) { print("WRONG at iter " + i + ": " + r); break; }
    } catch(e) {
        print("ERROR at iter " + i + ": " + e);
        break;
    }
}
print("Done");
