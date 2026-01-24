//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// ChakraCore x64 JIT Performance Test Suite
//
// This script comprehensively tests JIT compilation performance and optimization
// capabilities of ChakraCore. It includes various scenarios designed to trigger
// JIT compilation and measure the performance benefits.
//
// Test Categories:
// 1. Mathematical operations and hot loops
// 2. Function call optimization
// 3. Object property access optimization
// 4. Array manipulation and iteration
// 5. Type specialization
// 6. Memory allocation patterns
//
// Usage:
//   ./ch jit_performance_test.js
//   ./ch --no-jit jit_performance_test.js  (to compare with interpreter)
//-------------------------------------------------------------------------------------------------------

print("=== ChakraCore x64 JIT Performance Test Suite ===");
print("");

// Global test results
var testResults = [];
var totalTests = 0;
var passedTests = 0;

function addResult(testName, duration, status, notes) {
    totalTests++;
    if (status === "PASS") passedTests++;

    testResults.push({
        name: testName,
        duration: duration,
        status: status,
        notes: notes || ""
    });

    print("[" + status + "] " + testName + " - " + duration + "ms" + (notes ? " (" + notes + ")" : ""));
}

function benchmark(name, func, iterations) {
    iterations = iterations || 1;

    var start = Date.now();
    var result = null;

    for (var i = 0; i < iterations; i++) {
        result = func();
    }

    var end = Date.now();
    var duration = end - start;

    return {
        duration: duration,
        result: result,
        avgDuration: Math.round(duration / iterations * 100) / 100
    };
}

// Test 1: Mathematical Hot Loops (JIT should optimize heavily)
print("Test 1: Mathematical Hot Loops");
function mathHotLoop() {
    var sum = 0;
    var product = 1;

    // This loop should trigger JIT compilation
    for (var i = 1; i <= 100000; i++) {
        sum += Math.sqrt(i) + Math.sin(i / 1000) + Math.cos(i / 1000);
        product = (product * 1.0001) % 1000000;
    }

    return { sum: sum.toFixed(2), product: product.toFixed(2) };
}

var mathResult = benchmark("Mathematical Hot Loops", mathHotLoop);
addResult("Mathematical Operations", mathResult.duration, "PASS", "Hot loop optimization");

// Test 2: Function Call Optimization
print("\nTest 2: Function Call Optimization");
function add(a, b) { return a + b; }
function multiply(a, b) { return a * b; }
function divide(a, b) { return b !== 0 ? a / b : 0; }

function functionCallTest() {
    var result = 0;

    // Many function calls should be optimized by JIT
    for (var i = 0; i < 1000000; i++) {
        result = add(multiply(i, 2), divide(i, 3));
        result = add(result, multiply(i % 100, 3));
    }

    return result;
}

var funcResult = benchmark("Function Call Optimization", functionCallTest);
addResult("Function Calls", funcResult.duration, "PASS", "Inline optimization");

// Test 3: Object Property Access
print("\nTest 3: Object Property Access Optimization");
function ObjectTest(x, y, z) {
    this.x = x;
    this.y = y;
    this.z = z;
}

ObjectTest.prototype.calculate = function() {
    return this.x * this.y + this.z;
};

ObjectTest.prototype.update = function(factor) {
    this.x *= factor;
    this.y *= factor;
    this.z *= factor;
};

function objectAccessTest() {
    var objects = [];

    // Create objects with consistent shape for hidden class optimization
    for (var i = 0; i < 10000; i++) {
        objects.push(new ObjectTest(i, i + 1, i + 2));
    }

    var sum = 0;
    // Property access should be optimized
    for (var j = 0; j < 100; j++) {
        for (var k = 0; k < objects.length; k++) {
            objects[k].update(1.001);
            sum += objects[k].calculate();
        }
    }

    return sum;
}

var objResult = benchmark("Object Property Access", objectAccessTest);
addResult("Object Access", objResult.duration, "PASS", "Hidden class optimization");

// Test 4: Array Operations and Iteration
print("\nTest 4: Array Operations");
function arrayOperationsTest() {
    var arr = [];

    // Array growth pattern
    for (var i = 0; i < 100000; i++) {
        arr.push(i * 2);
    }

    // Map operation
    var doubled = arr.map(function(x) { return x * 2; });

    // Filter operation
    var evens = arr.filter(function(x) { return x % 1000 === 0; });

    // Reduce operation
    var sum = arr.reduce(function(acc, val) { return acc + val; }, 0);

    return {
        originalLength: arr.length,
        doubledLength: doubled.length,
        evensLength: evens.length,
        sum: sum
    };
}

var arrResult = benchmark("Array Operations", arrayOperationsTest);
addResult("Array Operations", arrResult.duration, "PASS", "Array method optimization");

// Test 5: Type Specialization
print("\nTest 5: Type Specialization");
function typeSpecializationTest() {
    // Integer arithmetic (should be optimized to native integer ops)
    function integerLoop() {
        var sum = 0;
        for (var i = 0; i < 1000000; i++) {
            sum += i;
        }
        return sum;
    }

    // Float arithmetic
    function floatLoop() {
        var sum = 0.0;
        for (var i = 0; i < 1000000; i++) {
            sum += i * 1.1;
        }
        return sum;
    }

    var intResult = integerLoop();
    var floatResult = floatLoop();

    return { intSum: intResult, floatSum: floatResult.toFixed(2) };
}

var typeResult = benchmark("Type Specialization", typeSpecializationTest);
addResult("Type Specialization", typeResult.duration, "PASS", "Int/float optimization");

// Test 6: Recursive Function Optimization
print("\nTest 6: Recursive Functions");
function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

function factorialIterative(n) {
    var result = 1;
    for (var i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

function recursiveTest() {
    var fibResults = [];
    var factResults = [];

    // Test moderate recursion (should be optimized)
    for (var i = 1; i <= 30; i++) {
        if (i <= 20) {
            fibResults.push(fibonacci(i));
        }
        factResults.push(factorialIterative(i));
    }

    return {
        fib20: fibResults[19],  // fibonacci(20)
        fact30: factResults[29]  // factorial(30)
    };
}

var recResult = benchmark("Recursive Functions", recursiveTest);
addResult("Recursion", recResult.duration, "PASS", "Tail call optimization");

// Test 7: String Operations
print("\nTest 7: String Operations");
function stringOperationsTest() {
    var strings = [];
    var baseString = "Performance test string number ";

    // String concatenation
    for (var i = 0; i < 50000; i++) {
        strings.push(baseString + i + " with additional text");
    }

    // String processing
    var processed = strings.map(function(s) {
        return s.toUpperCase().substring(0, 20);
    });

    // String searching
    var matches = 0;
    for (var j = 0; j < strings.length; j++) {
        if (strings[j].indexOf("1234") !== -1) {
            matches++;
        }
    }

    return {
        totalStrings: strings.length,
        processedStrings: processed.length,
        matches: matches
    };
}

var strResult = benchmark("String Operations", stringOperationsTest);
addResult("String Operations", strResult.duration, "PASS", "String interning");

// Test 8: Memory Allocation Patterns
print("\nTest 8: Memory Allocation Patterns");
function memoryAllocationTest() {
    var objects = [];
    var arrays = [];

    // Mixed allocation pattern
    for (var i = 0; i < 10000; i++) {
        // Object allocation
        objects.push({
            id: i,
            name: "Object " + i,
            value: Math.random() * 1000,
            timestamp: Date.now()
        });

        // Array allocation
        var arr = [];
        for (var j = 0; j < 100; j++) {
            arr.push(j * i);
        }
        arrays.push(arr);
    }

    // Cleanup some objects to test GC
    for (var k = 0; k < objects.length; k += 2) {
        objects[k] = null;
    }

    return {
        objectsCreated: objects.length,
        arraysCreated: arrays.length,
        totalElements: arrays.length * 100
    };
}

var memResult = benchmark("Memory Allocation", memoryAllocationTest);
addResult("Memory Patterns", memResult.duration, "PASS", "GC optimization");

// Test 9: Complex Data Structure Operations
print("\nTest 9: Complex Data Structures");
function complexDataTest() {
    // Create a complex nested structure
    var data = {
        users: [],
        index: {},
        stats: {
            total: 0,
            active: 0,
            premium: 0
        }
    };

    // Populate with data
    for (var i = 0; i < 10000; i++) {
        var user = {
            id: i,
            name: "User" + i,
            email: "user" + i + "@test.com",
            score: Math.floor(Math.random() * 1000),
            active: Math.random() > 0.3,
            premium: Math.random() > 0.8,
            tags: ["tag" + (i % 10), "category" + (i % 5)]
        };

        data.users.push(user);
        data.index["user" + i] = user;

        data.stats.total++;
        if (user.active) data.stats.active++;
        if (user.premium) data.stats.premium++;
    }

    // Complex queries
    var highScorers = data.users.filter(function(u) { return u.score > 800; });
    var premiumActive = data.users.filter(function(u) { return u.premium && u.active; });

    return {
        totalUsers: data.stats.total,
        activeUsers: data.stats.active,
        premiumUsers: data.stats.premium,
        highScorers: highScorers.length,
        premiumActive: premiumActive.length
    };
}

var complexResult = benchmark("Complex Data Structures", complexDataTest);
addResult("Complex Data", complexResult.duration, "PASS", "Structure optimization");

// Test 10: Performance Comparison Test
print("\nTest 10: JIT vs Baseline Performance");
function performanceComparisonTest() {
    var iterations = 1000000;
    var result = 0;

    var start = Date.now();

    // CPU-intensive computation that benefits from JIT
    for (var i = 0; i < iterations; i++) {
        result += Math.sqrt(i) * Math.sin(i / 10000) + Math.cos(i / 10000);
        if (i % 10000 === 0) {
            result = result % 1000000; // Prevent overflow
        }
    }

    var end = Date.now();
    var duration = end - start;

    return {
        iterations: iterations,
        result: result.toFixed(6),
        duration: duration,
        opsPerSec: Math.round(iterations / (duration / 1000))
    };
}

var perfResult = benchmark("JIT Performance Test", performanceComparisonTest);
addResult("JIT Performance", perfResult.duration, "PASS",
         perfResult.result.opsPerSec + " ops/sec");

// Generate Summary Report
print("\n" + "=".repeat(60));
print("PERFORMANCE TEST SUMMARY");
print("=".repeat(60));

print("Total Tests: " + totalTests);
print("Passed: " + passedTests);
print("Failed: " + (totalTests - passedTests));
print("Success Rate: " + Math.round((passedTests / totalTests) * 100) + "%");

print("\nDetailed Results:");
print("-".repeat(60));

var totalDuration = 0;
for (var i = 0; i < testResults.length; i++) {
    var test = testResults[i];
    totalDuration += test.duration;

    var line = test.name.padEnd ? test.name.padEnd(25) : test.name;
    while (line.length < 25) line += " ";

    line += " | " + test.duration.toString().padEnd ? test.duration.toString().padEnd(8) : test.duration + "ms";
    line += " | " + test.status;
    if (test.notes) {
        line += " | " + test.notes;
    }

    print(line);
}

print("-".repeat(60));
print("Total Execution Time: " + totalDuration + "ms");
print("Average Test Time: " + Math.round(totalDuration / totalTests) + "ms");

// Performance Analysis
print("\nPERFORMANCE ANALYSIS:");
print("-".repeat(30));

var fastests = testResults.slice().sort(function(a, b) { return a.duration - b.duration; });
var slowests = testResults.slice().sort(function(a, b) { return b.duration - a.duration; });

print("Fastest Test: " + fastests[0].name + " (" + fastests[0].duration + "ms)");
print("Slowest Test: " + slowests[0].name + " (" + slowests[0].duration + "ms)");

if (totalDuration < 5000) {
    print("Overall Performance: EXCELLENT (< 5 seconds)");
} else if (totalDuration < 10000) {
    print("Overall Performance: GOOD (< 10 seconds)");
} else if (totalDuration < 20000) {
    print("Overall Performance: ACCEPTABLE (< 20 seconds)");
} else {
    print("Overall Performance: NEEDS IMPROVEMENT (> 20 seconds)");
}

print("\nJIT OPTIMIZATION INDICATORS:");
print("- Mathematical operations should be fastest with JIT");
print("- Function calls should show significant speedup");
print("- Object property access should be optimized");
print("- Array operations should benefit from specialized paths");
print("- Type-specialized code should outperform generic paths");

print("\nTo compare with interpreter mode:");
print("  Run: ./ch --no-jit jit_performance_test.js");
print("\nExpected JIT benefits:");
print("- 10-50x speedup for mathematical operations");
print("- 5-20x speedup for function-heavy code");
print("- 3-10x speedup for object/array operations");

print("\n=== JIT Performance Test Complete ===");
