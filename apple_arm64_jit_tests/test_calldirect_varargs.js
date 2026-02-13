//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// Test for Apple ARM64 CallDirect varargs bug (Bug A)
// Tests that JIT-compiled code correctly calls C++ variadic Entry* helpers
// that use va_start on DarwinPCS (Apple Silicon).
//
// Expected behavior: All tests should pass without crashes or TypeErrors
// when JIT is enabled.
//

print("=== Apple ARM64 CallDirect Varargs Test Suite ===\n");

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

function assertThrows(fn, message) {
    testCount++;
    try {
        fn();
        failCount++;
        print("FAIL: " + message + " - did not throw");
    } catch (e) {
        passCount++;
        print("PASS: " + message);
    }
}

// Force JIT compilation by running in a hot loop
function forceJIT(fn, iterations) {
    iterations = iterations || 10000;
    for (var i = 0; i < iterations; i++) {
        fn();
    }
}

print("\n--- TypedArray Tests (CallDirect to variadic Entry* helpers) ---\n");

// Test 1: TypedArray indexOf
function testTypedArrayIndexOf() {
    var arr = new Int32Array([1, 2, 3, 4, 5]);
    var result = arr.indexOf(3);
    assert(result === 2, "TypedArray indexOf: found element at index 2");
}
forceJIT(testTypedArrayIndexOf);
testTypedArrayIndexOf();

// Test 2: TypedArray lastIndexOf
function testTypedArrayLastIndexOf() {
    var arr = new Int32Array([1, 2, 3, 2, 1]);
    var result = arr.lastIndexOf(2);
    assert(result === 3, "TypedArray lastIndexOf: found element at index 3");
}
forceJIT(testTypedArrayLastIndexOf);
testTypedArrayLastIndexOf();

// Test 3: TypedArray includes
function testTypedArrayIncludes() {
    var arr = new Uint8Array([10, 20, 30]);
    var result = arr.includes(20);
    assert(result === true, "TypedArray includes: found element");
}
forceJIT(testTypedArrayIncludes);
testTypedArrayIncludes();

// Test 4: TypedArray fill
function testTypedArrayFill() {
    var arr = new Float32Array([1, 2, 3, 4, 5]);
    arr.fill(0, 1, 4);
    assert(arr[0] === 1 && arr[1] === 0 && arr[2] === 0 && arr[3] === 0 && arr[4] === 5,
           "TypedArray fill: filled correctly");
}
forceJIT(testTypedArrayFill);
testTypedArrayFill();

// Test 5: TypedArray copyWithin
function testTypedArrayCopyWithin() {
    var arr = new Int16Array([1, 2, 3, 4, 5]);
    arr.copyWithin(0, 3);
    assert(arr[0] === 4 && arr[1] === 5, "TypedArray copyWithin: copied correctly");
}
forceJIT(testTypedArrayCopyWithin);
testTypedArrayCopyWithin();

// Test 6: TypedArray subarray
function testTypedArraySubarray() {
    var arr = new Uint32Array([1, 2, 3, 4, 5]);
    var sub = arr.subarray(1, 4);
    assert(sub.length === 3 && sub[0] === 2 && sub[2] === 4, "TypedArray subarray: created correctly");
}
forceJIT(testTypedArraySubarray);
testTypedArraySubarray();

print("\n--- Array Tests (CallDirect to variadic Entry* helpers) ---\n");

// Test 7: Array indexOf
function testArrayIndexOf() {
    var arr = [10, 20, 30, 40, 50];
    var result = arr.indexOf(30);
    assert(result === 2, "Array indexOf: found element at index 2");
}
forceJIT(testArrayIndexOf);
testArrayIndexOf();

// Test 8: Array lastIndexOf
function testArrayLastIndexOf() {
    var arr = [1, 2, 3, 2, 1];
    var result = arr.lastIndexOf(2);
    assert(result === 3, "Array lastIndexOf: found element at index 3");
}
forceJIT(testArrayLastIndexOf);
testArrayLastIndexOf();

// Test 9: Array includes
function testArrayIncludes() {
    var arr = ["a", "b", "c"];
    var result = arr.includes("b");
    assert(result === true, "Array includes: found element");
}
forceJIT(testArrayIncludes);
testArrayIncludes();

// Test 10: Array fill
function testArrayFill() {
    var arr = [1, 2, 3, 4, 5];
    arr.fill(0, 2, 4);
    assert(arr[0] === 1 && arr[2] === 0 && arr[3] === 0 && arr[4] === 5, "Array fill: filled correctly");
}
forceJIT(testArrayFill);
testArrayFill();

// Test 11: Array copyWithin
function testArrayCopyWithin() {
    var arr = [1, 2, 3, 4, 5];
    arr.copyWithin(0, 3, 5);
    assert(arr[0] === 4 && arr[1] === 5, "Array copyWithin: copied correctly");
}
forceJIT(testArrayCopyWithin);
testArrayCopyWithin();

print("\n--- String Tests (CallDirect to variadic Entry* helpers) ---\n");

// Test 12: String indexOf
function testStringIndexOf() {
    var str = "hello world";
    var result = str.indexOf("world");
    assert(result === 6, "String indexOf: found substring at index 6");
}
forceJIT(testStringIndexOf);
testStringIndexOf();

// Test 13: String lastIndexOf
function testStringLastIndexOf() {
    var str = "hello hello";
    var result = str.lastIndexOf("hello");
    assert(result === 6, "String lastIndexOf: found substring at index 6");
}
forceJIT(testStringLastIndexOf);
testStringLastIndexOf();

// Test 14: String includes
function testStringIncludes() {
    var str = "JavaScript";
    var result = str.includes("Script");
    assert(result === true, "String includes: found substring");
}
forceJIT(testStringIncludes);
testStringIncludes();

// Test 15: String slice
function testStringSlice() {
    var str = "JavaScript";
    var result = str.slice(4, 10);
    assert(result === "Script", "String slice: extracted substring");
}
forceJIT(testStringSlice);
testStringSlice();

// Test 16: String substring
function testStringSubstring() {
    var str = "JavaScript";
    var result = str.substring(4, 10);
    assert(result === "Script", "String substring: extracted substring");
}
forceJIT(testStringSubstring);
testStringSubstring();

// Test 17: String substr
function testStringSubstr() {
    var str = "JavaScript";
    var result = str.substr(4, 6);
    assert(result === "Script", "String substr: extracted substring");
}
forceJIT(testStringSubstr);
testStringSubstr();

print("\n--- RegExp Tests (CallDirect to variadic Entry* helpers) ---\n");

// Test 18: RegExp exec
function testRegExpExec() {
    var re = /world/;
    var result = re.exec("hello world");
    assert(result !== null && result[0] === "world", "RegExp exec: matched pattern");
}
forceJIT(testRegExpExec);
testRegExpExec();

// Test 19: RegExp test
function testRegExpTest() {
    var re = /\d+/;
    var result = re.test("abc123def");
    assert(result === true, "RegExp test: matched pattern");
}
forceJIT(testRegExpTest);
testRegExpTest();

// Test 20: String match
function testStringMatch() {
    var str = "The year is 2024";
    var result = str.match(/\d+/);
    assert(result !== null && result[0] === "2024", "String match: found pattern");
}
forceJIT(testStringMatch);
testStringMatch();

// Test 21: String search
function testStringSearch() {
    var str = "hello world";
    var result = str.search(/world/);
    assert(result === 6, "String search: found pattern at index 6");
}
forceJIT(testStringSearch);
testStringSearch();

// Test 22: String replace
function testStringReplace() {
    var str = "hello world";
    var result = str.replace("world", "JavaScript");
    assert(result === "hello JavaScript", "String replace: replaced substring");
}
forceJIT(testStringReplace);
testStringReplace();

print("\n--- Mixed Variadic Argument Tests ---\n");

// Test 23: Multiple optional arguments
function testMultipleOptionalArgs() {
    var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    var result = arr.slice(2, 8);
    assert(result.length === 6 && result[0] === 3 && result[5] === 8,
           "Array slice with optional args: extracted correctly");
}
forceJIT(testMultipleOptionalArgs);
testMultipleOptionalArgs();

// Test 24: Missing optional arguments
function testMissingOptionalArgs() {
    var arr = [1, 2, 3, 4, 5];
    var result = arr.indexOf(3);
    assert(result === 2, "indexOf with default fromIndex: found element");
}
forceJIT(testMissingOptionalArgs);
testMissingOptionalArgs();

// Test 25: Negative indices
function testNegativeIndices() {
    var arr = [1, 2, 3, 4, 5];
    var result = arr.indexOf(4, -3);
    assert(result === 3, "indexOf with negative fromIndex: found element");
}
forceJIT(testNegativeIndices);
testNegativeIndices();

print("\n--- Stress Tests (Heavy JIT) ---\n");

// Test 26: Repeated calls in tight loop
function testRepeatedCalls() {
    var arr = new Int32Array(100);
    for (var i = 0; i < 100; i++) {
        arr[i] = i;
    }

    var successCount = 0;
    for (var i = 0; i < 1000; i++) {
        if (arr.indexOf(50) === 50) successCount++;
    }
    assert(successCount === 1000, "Repeated TypedArray indexOf calls: all succeeded");
}
forceJIT(testRepeatedCalls, 5000);
testRepeatedCalls();

// Test 27: Mixed types in loop
function testMixedTypesLoop() {
    var int32arr = new Int32Array([1, 2, 3]);
    var float32arr = new Float32Array([1.5, 2.5, 3.5]);
    var normalArr = [10, 20, 30];

    var successCount = 0;
    for (var i = 0; i < 300; i++) {
        if (int32arr.indexOf(2) === 1) successCount++;
        if (float32arr.indexOf(2.5) === 1) successCount++;
        if (normalArr.indexOf(20) === 1) successCount++;
    }
    assert(successCount === 900, "Mixed type indexOf calls: all succeeded");
}
forceJIT(testMixedTypesLoop, 5000);
testMixedTypesLoop();

// Test 28: Nested calls
function testNestedCalls() {
    var arr1 = [1, 2, 3, 4, 5];
    var arr2 = ["a", "b", "c"];

    var result = arr1.slice(arr2.indexOf("b"), arr2.indexOf("c") + 2);
    assert(result.length === 2 && result[0] === 2 && result[1] === 3,
           "Nested variadic calls: computed correctly");
}
forceJIT(testNestedCalls);
testNestedCalls();

print("\n--- Edge Cases ---\n");

// Test 29: Empty arrays
function testEmptyArrays() {
    var arr = new Int32Array(0);
    var result = arr.indexOf(1);
    assert(result === -1, "Empty TypedArray indexOf: returned -1");
}
forceJIT(testEmptyArrays);
testEmptyArrays();

// Test 30: Large arrays
function testLargeArrays() {
    var arr = new Int32Array(10000);
    for (var i = 0; i < 10000; i++) {
        arr[i] = i;
    }
    var result = arr.indexOf(9999);
    assert(result === 9999, "Large TypedArray indexOf: found element");
}
forceJIT(testLargeArrays);
testLargeArrays();

// Test 31: Not found
function testNotFound() {
    var arr = new Uint8Array([1, 2, 3]);
    var result = arr.indexOf(99);
    assert(result === -1, "TypedArray indexOf not found: returned -1");
}
forceJIT(testNotFound);
testNotFound();

// Test 32: Boundary conditions
function testBoundaryConditions() {
    var arr = new Int32Array([0, 1, 2, 3, 4]);
    var first = arr.indexOf(0);
    var last = arr.indexOf(4);
    assert(first === 0 && last === 4, "TypedArray indexOf boundaries: found first and last");
}
forceJIT(testBoundaryConditions);
testBoundaryConditions();

print("\n=== Test Results ===");
print("Total tests: " + testCount);
print("Passed: " + passCount);
print("Failed: " + failCount);

if (failCount === 0) {
    print("\n✓ ALL TESTS PASSED - CallDirect varargs bug is FIXED!");
} else {
    print("\n✗ SOME TESTS FAILED - CallDirect varargs bug still present");
}
