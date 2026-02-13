// Test: Single call to 7-param function (interpreter only, no JIT)
function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }
print("Before single call...");
var r = test7p(1, 2, 3, 4, 5, 6, 7);
print("After single call: " + r);
if (r !== 28) print("WRONG!"); else print("PASS: single call OK");
