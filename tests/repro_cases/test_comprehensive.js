// Bug B: 7-param test
function test7p(a, b, c, d, e, f, g) { return a + b + c + d + e + f + g; }
for (var i = 0; i < 2000; i++) {
    var r = test7p(1, 2, 3, 4, 5, 6, 7);
    if (r !== 28) { print("FAIL 7p: " + r); throw "stop"; }
}
print("PASS: 7-param (28)");

// Bug B: 8-param test
function test8p(a, b, c, d, e, f, g, h) { return a + b + c + d + e + f + g + h; }
for (var i = 0; i < 2000; i++) {
    var r = test8p(1, 2, 3, 4, 5, 6, 7, 8);
    if (r !== 36) { print("FAIL 8p: " + r); throw "stop"; }
}
print("PASS: 8-param (36)");

// Bug B: 10-param test
function test10p(a, b, c, d, e, f, g, h, ii, j) { return a + b + c + d + e + f + g + h + ii + j; }
for (var i = 0; i < 2000; i++) {
    var r = test10p(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    if (r !== 55) { print("FAIL 10p: " + r); throw "stop"; }
}
print("PASS: 10-param (55)");

// Bug B: 6-param (no overflow, should still work)
function test6p(a, b, c, d, e, f) { return a + b + c + d + e + f; }
for (var i = 0; i < 2000; i++) {
    var r = test6p(1, 2, 3, 4, 5, 6);
    if (r !== 21) { print("FAIL 6p: " + r); throw "stop"; }
}
print("PASS: 6-param (21)");

// Bug A: TypedArray.indexOf (variadic C++ callee)
function testTA_indexOf() {
    var arr = new Int32Array(4);
    arr[0] = 42;
    var r = -1;
    for (var j = 0; j < 10; j++) { r = arr.indexOf(42); }
    return r;
}
for (var i = 0; i < 200; i++) {
    var r = testTA_indexOf();
    if (r !== 0) { print("FAIL TA.indexOf: " + r); throw "stop"; }
}
print("PASS: TypedArray.indexOf");

// Bug A: TypedArray.fill
function testTA_fill() {
    var arr = new Int32Array(16);
    for (var i = 0; i < 200; i++) { arr.fill(99); }
    return arr[0];
}
var r2 = testTA_fill();
if (r2 !== 99) { print("FAIL TA.fill: " + r2); throw "stop"; }
print("PASS: TypedArray.fill");

// Bug A: TypedArray.includes
function testTA_includes() {
    var arr = new Int32Array(8);
    arr[3] = 77;
    for (var i = 0; i < 200; i++) {
        if (!arr.includes(77)) { print("FAIL TA.includes at iter " + i); throw "stop"; }
    }
}
testTA_includes();
print("PASS: TypedArray.includes");

// Combined: 7-param + builtin call inside
function testCombined(a, b, c, d, e, f, g) {
    var arr = new Int32Array(4);
    arr[0] = g;
    return arr.indexOf(g);
}
for (var i = 0; i < 2000; i++) {
    var r3 = testCombined(1, 2, 3, 4, 5, 6, 42);
    if (r3 !== 0) { print("FAIL combined: " + r3); throw "stop"; }
}
print("PASS: Combined 7-param + indexOf");

// String.indexOf (variadic C++ callee)
function testStringIndexOf() {
    var s = "hello world";
    for (var i = 0; i < 200; i++) {
        var r = s.indexOf("world");
        if (r !== 6) { print("FAIL String.indexOf: " + r); throw "stop"; }
    }
}
testStringIndexOf();
print("PASS: String.indexOf");

// Array.indexOf
function testArrayIndexOf() {
    var arr = [10, 20, 30, 40, 50];
    for (var i = 0; i < 200; i++) {
        var r = arr.indexOf(30);
        if (r !== 2) { print("FAIL Array.indexOf: " + r); throw "stop"; }
    }
}
testArrayIndexOf();
print("PASS: Array.indexOf");

// Array.splice with many args (C++ variadic with >6 args from interpreter)
function testSplice() {
    for (var i = 0; i < 50; i++) {
        var a = [1, 2, 3];
        a.splice(1, 0, "a", "b", "c", "d", "e", "f");
        if (a.length !== 9) { print("FAIL splice len: " + a.length); throw "stop"; }
        if (a[1] !== "a") { print("FAIL splice val: " + a[1]); throw "stop"; }
    }
}
testSplice();
print("PASS: Array.splice with many args");

print("\nALL TESTS PASSED");
