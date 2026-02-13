//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// Test for Apple ARM64 JS→JS 7+ parameters bug (Bug B)
// Tests that JIT-compiled JS functions with 7 or more parameters
// correctly pass and receive arguments on DarwinPCS (Apple Silicon).
//
// Expected behavior: All tests should pass without crashes or incorrect values
// when JIT is enabled.
//

print("=== Apple ARM64 JS→JS 7+ Parameters Test Suite ===\n");

var passCount = 0;
var failCount = 0;
var testCount = 0;

function assert(condition, message) {
    testCount++;
    if (condition) {
        passCount++;
        print("PASS: " + message);
    } else {
        failCount++;
        print("FAIL: " + message);
    }
}

function assertNoThrow(fn, message) {
    testCount++;
    try {
        fn();
        passCount++;
        print("PASS: " + message);
    } catch (e) {
        failCount++;
        print("FAIL: " + message + " - threw: " + e);
    }
}

// Force JIT compilation by running in a hot loop
function forceJIT(fn, iterations) {
    iterations = iterations || 10000;
    for (var i = 0; i < iterations; i++) {
        fn();
    }
}

print("\n--- Basic 7+ Parameter Tests ---\n");

// Test 1: Exactly 7 parameters
function func7params(a, b, c, d, e, f, g) {
    return a + b + c + d + e + f + g;
}

function testFunc7Params() {
    var result = func7params(1, 2, 3, 4, 5, 6, 7);
    assert(result === 28, "7 parameters: sum = 28");
}
forceJIT(testFunc7Params);
testFunc7Params();

// Test 2: 8 parameters
function func8params(a, b, c, d, e, f, g, h) {
    return a + b + c + d + e + f + g + h;
}

function testFunc8Params() {
    var result = func8params(1, 2, 3, 4, 5, 6, 7, 8);
    assert(result === 36, "8 parameters: sum = 36");
}
forceJIT(testFunc8Params);
testFunc8Params();

// Test 3: 9 parameters
function func9params(a, b, c, d, e, f, g, h, i) {
    return a + b + c + d + e + f + g + h + i;
}

function testFunc9Params() {
    var result = func9params(1, 2, 3, 4, 5, 6, 7, 8, 9);
    assert(result === 45, "9 parameters: sum = 45");
}
forceJIT(testFunc9Params);
testFunc9Params();

// Test 4: 10 parameters
function func10params(a, b, c, d, e, f, g, h, i, j) {
    return a + b + c + d + e + f + g + h + i + j;
}

function testFunc10Params() {
    var result = func10params(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    assert(result === 55, "10 parameters: sum = 55");
}
forceJIT(testFunc10Params);
testFunc10Params();

// Test 5: 12 parameters
function func12params(a, b, c, d, e, f, g, h, i, j, k, l) {
    return a + b + c + d + e + f + g + h + i + j + k + l;
}

function testFunc12Params() {
    var result = func12params(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    assert(result === 78, "12 parameters: sum = 78");
}
forceJIT(testFunc12Params);
testFunc12Params();

// Test 6: 16 parameters
function func16params(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {
    return a + b + c + d + e + f + g + h + i + j + k + l + m + n + o + p;
}

function testFunc16Params() {
    var result = func16params(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    assert(result === 136, "16 parameters: sum = 136");
}
forceJIT(testFunc16Params);
testFunc16Params();

print("\n--- Individual Parameter Access Tests ---\n");

// Test 7: Access each parameter individually
function funcAccessEach(a, b, c, d, e, f, g, h, i, j) {
    assert(a === 1, "Parameter 1 (a) = 1");
    assert(b === 2, "Parameter 2 (b) = 2");
    assert(c === 3, "Parameter 3 (c) = 3");
    assert(d === 4, "Parameter 4 (d) = 4");
    assert(e === 5, "Parameter 5 (e) = 5");
    assert(f === 6, "Parameter 6 (f) = 6");
    assert(g === 7, "Parameter 7 (g) = 7");
    assert(h === 8, "Parameter 8 (h) = 8");
    assert(i === 9, "Parameter 9 (i) = 9");
    assert(j === 10, "Parameter 10 (j) = 10");
}

function testAccessEach() {
    funcAccessEach(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}
forceJIT(testAccessEach);
testAccessEach();

print("\n--- Different Data Types Tests ---\n");

// Test 8: Mixed integer and string parameters
function funcMixedTypes(a, b, c, d, e, f, g, h, i, j) {
    return a + b + c + d + e + f + g + "-" + h + "-" + i + "-" + j;
}

function testMixedTypes() {
    var result = funcMixedTypes(1, 2, 3, 4, 5, 6, 7, "eight", "nine", "ten");
    assert(result === "28-eight-nine-ten", "Mixed types: concatenated correctly");
}
forceJIT(testMixedTypes);
testMixedTypes();

// Test 9: Floating point parameters
function funcFloats(a, b, c, d, e, f, g, h, i, j) {
    return a + b + c + d + e + f + g + h + i + j;
}

function testFloats() {
    var result = funcFloats(1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1);
    var expected = 1.1 + 2.2 + 3.3 + 4.4 + 5.5 + 6.6 + 7.7 + 8.8 + 9.9 + 10.1;
    assert(Math.abs(result - expected) < 0.001, "Floating point params: sum correct");
}
forceJIT(testFloats);
testFloats();

// Test 10: Object parameters
function funcObjects(a, b, c, d, e, f, g, h) {
    return h.value;
}

function testObjects() {
    var obj = {value: 42};
    var result = funcObjects(1, 2, 3, 4, 5, 6, 7, obj);
    assert(result === 42, "Object parameter at position 8: accessed correctly");
}
forceJIT(testObjects);
testObjects();

// Test 11: Array parameters
function funcArrays(a, b, c, d, e, f, g, h) {
    return h[0] + h[1] + h[2];
}

function testArrays() {
    var arr = [10, 20, 30];
    var result = funcArrays(1, 2, 3, 4, 5, 6, 7, arr);
    assert(result === 60, "Array parameter at position 8: accessed correctly");
}
forceJIT(testArrays);
testArrays();

print("\n--- Return Value Tests ---\n");

// Test 12: Return 7th parameter
function funcReturn7th(a, b, c, d, e, f, g) {
    return g;
}

function testReturn7th() {
    var result = funcReturn7th(1, 2, 3, 4, 5, 6, 777);
    assert(result === 777, "Return 7th parameter: correct value");
}
forceJIT(testReturn7th);
testReturn7th();

// Test 13: Return 8th parameter
function funcReturn8th(a, b, c, d, e, f, g, h) {
    return h;
}

function testReturn8th() {
    var result = funcReturn8th(1, 2, 3, 4, 5, 6, 7, 888);
    assert(result === 888, "Return 8th parameter: correct value");
}
forceJIT(testReturn8th);
testReturn8th();

// Test 14: Return 10th parameter
function funcReturn10th(a, b, c, d, e, f, g, h, i, j) {
    return j;
}

function testReturn10th() {
    var result = funcReturn10th(1, 2, 3, 4, 5, 6, 7, 8, 9, 1010);
    assert(result === 1010, "Return 10th parameter: correct value");
}
forceJIT(testReturn10th);
testReturn10th();

print("\n--- Nested Call Tests ---\n");

// Test 15: Nested calls with 7+ parameters
function funcOuter(a, b, c, d, e, f, g, h) {
    return funcInner(a, b, c, d, e, f, g, h);
}

function funcInner(a, b, c, d, e, f, g, h) {
    return a + b + c + d + e + f + g + h;
}

function testNestedCalls() {
    var result = funcOuter(1, 2, 3, 4, 5, 6, 7, 8);
    assert(result === 36, "Nested 8-param calls: sum = 36");
}
forceJIT(testNestedCalls);
testNestedCalls();

// Test 16: Recursive call with 7+ parameters
function funcRecursive(a, b, c, d, e, f, g, depth) {
    if (depth <= 0) {
        return a + b + c + d + e + f + g;
    }
    return funcRecursive(a, b, c, d, e, f, g, depth - 1);
}

function testRecursiveCalls() {
    var result = funcRecursive(1, 2, 3, 4, 5, 6, 7, 5);
    assert(result === 28, "Recursive 8-param calls: sum = 28");
}
forceJIT(testRecursiveCalls);
testRecursiveCalls();

print("\n--- Stress Tests (Heavy JIT) ---\n");

// Test 17: Hot loop calling 7-param function
function testHotLoop7Params() {
    var total = 0;
    for (var i = 0; i < 1000; i++) {
        total += func7params(1, 2, 3, 4, 5, 6, 7);
    }
    assert(total === 28000, "Hot loop 7-param calls: total = 28000");
}
forceJIT(testHotLoop7Params, 5000);
testHotLoop7Params();

// Test 18: Hot loop calling 10-param function
function testHotLoop10Params() {
    var total = 0;
    for (var i = 0; i < 1000; i++) {
        total += func10params(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    }
    assert(total === 55000, "Hot loop 10-param calls: total = 55000");
}
forceJIT(testHotLoop10Params, 5000);
testHotLoop10Params();

// Test 19: Mixed parameter counts in loop
function testMixedParamCounts() {
    var total = 0;
    for (var i = 0; i < 300; i++) {
        total += func7params(1, 2, 3, 4, 5, 6, 7);
        total += func8params(1, 2, 3, 4, 5, 6, 7, 8);
        total += func10params(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    }
    assert(total === 36300, "Mixed param count loop: total = 36300");
}
forceJIT(testMixedParamCounts, 5000);
testMixedParamCounts();

print("\n--- Edge Cases ---\n");

// Test 20: Undefined parameters
function funcUndefined(a, b, c, d, e, f, g, h) {
    return h === undefined;
}

function testUndefined() {
    var result = funcUndefined(1, 2, 3, 4, 5, 6, 7);
    assert(result === true, "8th parameter undefined when not passed: correct");
}
forceJIT(testUndefined);
testUndefined();

// Test 21: Null parameters
function funcNull(a, b, c, d, e, f, g, h) {
    return h === null;
}

function testNull() {
    var result = funcNull(1, 2, 3, 4, 5, 6, 7, null);
    assert(result === true, "8th parameter null: correct");
}
forceJIT(testNull);
testNull();

// Test 22: Boolean parameters
function funcBooleans(a, b, c, d, e, f, g, h, i, j) {
    return g && h && !i && !j;
}

function testBooleans() {
    var result = funcBooleans(1, 2, 3, 4, 5, 6, true, true, false, false);
    assert(result === true, "Boolean parameters 7-10: correct");
}
forceJIT(testBooleans);
testBooleans();

// Test 23: Large numbers
function funcLargeNumbers(a, b, c, d, e, f, g, h) {
    return h - g;
}

function testLargeNumbers() {
    var result = funcLargeNumbers(1, 2, 3, 4, 5, 6, 1000000, 2000000);
    assert(result === 1000000, "Large number parameters 7-8: correct difference");
}
forceJIT(testLargeNumbers);
testLargeNumbers();

// Test 24: Negative numbers
function funcNegativeNumbers(a, b, c, d, e, f, g, h, i) {
    return g + h + i;
}

function testNegativeNumbers() {
    var result = funcNegativeNumbers(1, 2, 3, 4, 5, 6, -10, -20, -30);
    assert(result === -60, "Negative number parameters 7-9: sum = -60");
}
forceJIT(testNegativeNumbers);
testNegativeNumbers();

print("\n--- Arguments Object Tests ---\n");

// Test 25: Arguments object access
function funcArguments(a, b, c, d, e, f, g, h, i, j) {
    return arguments[7] + arguments[8] + arguments[9];
}

function testArgumentsObject() {
    var result = funcArguments(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    assert(result === 27, "Arguments object access [7-9]: sum = 27");
}
forceJIT(testArgumentsObject);
testArgumentsObject();

// Test 26: Arguments length
function funcArgumentsLength(a, b, c, d, e, f, g, h) {
    return arguments.length;
}

function testArgumentsLength() {
    var result = funcArgumentsLength(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    assert(result === 10, "Arguments length with 10 params: correct");
}
forceJIT(testArgumentsLength);
testArgumentsLength();

print("\n--- Function Call Methods Tests ---\n");

// Test 27: apply with 8+ arguments
function testApply() {
    var args = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    var result = func10params.apply(null, args);
    assert(result === 55, "apply with 10 args: sum = 55");
}
forceJIT(testApply);
testApply();

// Test 28: call with 8+ arguments
function testCall() {
    var result = func10params.call(null, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    assert(result === 55, "call with 10 args: sum = 55");
}
forceJIT(testCall);
testCall();

print("\n--- Constructor Tests ---\n");

// Test 29: Constructor with 7+ parameters
function Constructor7Plus(a, b, c, d, e, f, g, h) {
    this.a = a;
    this.b = b;
    this.c = c;
    this.d = d;
    this.e = e;
    this.f = f;
    this.g = g;
    this.h = h;
}

function testConstructor() {
    var obj = new Constructor7Plus(1, 2, 3, 4, 5, 6, 7, 8);
    assert(obj.g === 7 && obj.h === 8, "Constructor with 8 params: properties 7-8 correct");
}
forceJIT(testConstructor);
testConstructor();

print("\n--- Method Call Tests ---\n");

// Test 30: Method with 7+ parameters
var obj = {
    method: function(a, b, c, d, e, f, g, h) {
        return a + b + c + d + e + f + g + h;
    }
};

function testMethod() {
    var result = obj.method(1, 2, 3, 4, 5, 6, 7, 8);
    assert(result === 36, "Object method with 8 params: sum = 36");
}
forceJIT(testMethod);
testMethod();

print("\n=== Test Results ===");
print("Total tests: " + testCount);
print("Passed: " + passCount);
print("Failed: " + failCount);

if (failCount === 0) {
    print("\n✓ ALL TESTS PASSED - JS→JS 7+ parameters bug is FIXED!");
} else {
    print("\n✗ SOME TESTS FAILED - JS→JS 7+ parameters bug still present");
}
