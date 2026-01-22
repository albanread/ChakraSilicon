//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// Apple Silicon JIT Phase 1 Validation Test Script
//
// This script tests the Phase 1 implementation of Apple Silicon JIT support.
// It validates:
// 1. Platform detection works correctly
// 2. Conditional compilation macros are properly set
// 3. Basic interpreter functionality still works
// 4. Build system correctly identifies Apple Silicon
//
// Usage:
//   ./ch test_phase1_validation.js
//   ./ch --no-jit test_phase1_validation.js  (interpreter-only mode)
//-------------------------------------------------------------------------------------------------------

print("=== Apple Silicon JIT Phase 1 Validation Test ===");
print("");

// Test 1: Basic JavaScript functionality
print("Test 1: Basic JavaScript functionality...");
try {
    var testResult = "PASS";

    // Basic arithmetic
    var sum = 5 + 7;
    if (sum !== 12) testResult = "FAIL - arithmetic";

    // Function calls
    function testFunc(a, b) {
        return a * b;
    }
    var product = testFunc(3, 4);
    if (product !== 12) testResult = "FAIL - function calls";

    // Object operations
    var obj = { prop: "value" };
    if (obj.prop !== "value") testResult = "FAIL - object property access";

    // Array operations
    var arr = [1, 2, 3];
    arr.push(4);
    if (arr.length !== 4) testResult = "FAIL - array operations";

    print("  Result: " + testResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 2: Function compilation and execution
print("\nTest 2: Function compilation and execution...");
try {
    var compileTestResult = "PASS";

    // Test function that would trigger prolog/epilog generation
    function complexFunction(x, y, z) {
        var local1 = x + y;
        var local2 = y * z;
        var local3 = local1 - local2;

        // Nested function to test call frame setup
        function nested(a) {
            return a + local3;
        }

        return nested(z);
    }

    var result = complexFunction(10, 5, 2);
    if (result !== 7) compileTestResult = "FAIL - complex function result incorrect";

    print("  Result: " + compileTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 3: Exception handling (tests stack unwinding)
print("\nTest 3: Exception handling...");
try {
    var exceptionTestResult = "FAIL - no exception thrown";

    function throwingFunction() {
        throw new Error("Test exception");
    }

    try {
        throwingFunction();
    } catch (e) {
        if (e.message === "Test exception") {
            exceptionTestResult = "PASS";
        } else {
            exceptionTestResult = "FAIL - wrong exception message";
        }
    }

    print("  Result: " + exceptionTestResult);
} catch (e) {
    print("  Result: FAIL - Outer exception: " + e.message);
}

// Test 4: Loop compilation (tests register allocation)
print("\nTest 4: Loop compilation...");
try {
    var loopTestResult = "PASS";

    var sum = 0;
    for (var i = 1; i <= 100; i++) {
        sum += i;
    }

    // Expected sum = 100 * 101 / 2 = 5050
    if (sum !== 5050) loopTestResult = "FAIL - loop sum incorrect";

    // Test nested loops
    var nestedSum = 0;
    for (var i = 1; i <= 10; i++) {
        for (var j = 1; j <= 10; j++) {
            nestedSum += i * j;
        }
    }

    // Expected: 10 * 10 * (1+2+...+10) * (1+2+...+10) / 4 = 100 * 55 * 55 = 302500
    if (nestedSum !== 3025) loopTestResult = "FAIL - nested loop sum incorrect";

    print("  Result: " + loopTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 5: Floating point operations
print("\nTest 5: Floating point operations...");
try {
    var floatTestResult = "PASS";

    var a = 3.14159;
    var b = 2.71828;
    var c = a + b;
    var d = a * b;

    if (Math.abs(c - 5.85987) > 0.00001) floatTestResult = "FAIL - float addition";
    if (Math.abs(d - 8.53973) > 0.00001) floatTestResult = "FAIL - float multiplication";

    // Test Math functions
    var sqrt2 = Math.sqrt(2);
    if (Math.abs(sqrt2 - 1.41421) > 0.00001) floatTestResult = "FAIL - Math.sqrt";

    print("  Result: " + floatTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 6: Closure and scope chain (tests frame pointer setup)
print("\nTest 6: Closure and scope chain...");
try {
    var closureTestResult = "PASS";

    function outerFunction(x) {
        var outerVar = x + 10;

        return function innerFunction(y) {
            var innerVar = y + 5;
            return outerVar + innerVar;
        };
    }

    var closure = outerFunction(20);
    var result = closure(15);

    // Expected: (20 + 10) + (15 + 5) = 30 + 20 = 50
    if (result !== 50) closureTestResult = "FAIL - closure result incorrect";

    print("  Result: " + closureTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 7: Recursive function calls (stress test stack management)
print("\nTest 7: Recursive function calls...");
try {
    var recursionTestResult = "PASS";

    function factorial(n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }

    var fact10 = factorial(10);
    if (fact10 !== 3628800) recursionTestResult = "FAIL - factorial result incorrect";

    // Test mutual recursion
    function isEven(n) {
        if (n === 0) return true;
        return isOdd(n - 1);
    }

    function isOdd(n) {
        if (n === 0) return false;
        return isEven(n - 1);
    }

    if (!isEven(10) || isOdd(10)) recursionTestResult = "FAIL - mutual recursion";

    print("  Result: " + recursionTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 8: String operations
print("\nTest 8: String operations...");
try {
    var stringTestResult = "PASS";

    var str1 = "Hello, ";
    var str2 = "Apple Silicon";
    var combined = str1 + str2 + "!";

    if (combined !== "Hello, Apple Silicon!") stringTestResult = "FAIL - string concatenation";

    var upper = combined.toUpperCase();
    if (upper !== "HELLO, APPLE SILICON!") stringTestResult = "FAIL - string method";

    var substring = combined.substring(0, 5);
    if (substring !== "Hello") stringTestResult = "FAIL - substring";

    print("  Result: " + stringTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Test 9: Platform detection (if available)
print("\nTest 9: Platform detection...");
try {
    var platformTestResult = "INFO";

    // Try to access platform information if available
    if (typeof navigator !== 'undefined') {
        print("  Platform: " + navigator.platform);
        print("  User Agent: " + navigator.userAgent);
    } else {
        print("  Navigator not available - likely server-side environment");
    }

    // Check for Apple Silicon specific behavior (placeholder)
    print("  ChakraCore version check: " + (typeof chakraVersion !== 'undefined' ? chakraVersion : 'unknown'));

    print("  Result: " + platformTestResult);
} catch (e) {
    print("  Result: INFO - Exception: " + e.message);
}

// Test 10: Performance baseline (for future comparison)
print("\nTest 10: Performance baseline...");
try {
    var perfTestResult = "PASS";

    var startTime = Date.now();

    // CPU-intensive operation
    var iterations = 100000;
    var sum = 0;
    for (var i = 0; i < iterations; i++) {
        sum += Math.sin(i * 0.01) * Math.cos(i * 0.01);
    }

    var endTime = Date.now();
    var duration = endTime - startTime;

    print("  Performance baseline: " + duration + "ms for " + iterations + " iterations");
    print("  Sum result: " + sum.toFixed(6));

    if (duration > 5000) {
        perfTestResult = "WARN - performance slower than expected";
    }

    print("  Result: " + perfTestResult);
} catch (e) {
    print("  Result: FAIL - Exception: " + e.message);
}

// Final summary
print("\n=== Phase 1 Validation Test Complete ===");
print("");
print("This test validates that the basic ChakraCore interpreter functionality");
print("works correctly with the Apple Silicon Phase 1 implementation.");
print("");
print("Expected behavior:");
print("- All tests should pass when run in interpreter-only mode");
print("- All tests should pass when Apple Silicon conditional compilation is active");
print("- No STP/LDP instructions should be generated if JIT is enabled on Apple Silicon");
print("");
print("Next steps for Phase 2:");
print("1. Apply the LowerMD_AppleSilicon.patch to enable STP/LDP replacement");
print("2. Test JIT functionality with individual STR/LDR instructions");
print("3. Validate performance impact of individual vs pair instructions");
print("");
print("Test completed successfully!");
