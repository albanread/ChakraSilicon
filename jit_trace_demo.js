//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// JIT Assembly Trace Demo Script
//
// This script demonstrates the JIT assembly tracing functionality integrated into ChakraCore.
// When run with the --TraceJitAsm flag, it will show detailed disassembly of compiled functions
// along with control flow analysis, register usage, and performance metrics.
//
// Usage:
//   ./ch --TraceJitAsm jit_trace_demo.js
//   CHAKRA_TRACE_JIT_ASM=1 ./ch jit_trace_demo.js
//
// Expected Output:
// - Function header with name and memory address
// - Complete x64 assembly disassembly with instruction-by-instruction analysis
// - Control flow summary (branches, calls, returns)
// - Register usage analysis showing hot registers
// - Performance metrics and optimization insights
//-------------------------------------------------------------------------------------------------------

print("=== ChakraCore JIT Assembly Trace Demonstration ===");
print("");
print("This demo showcases JIT compilation tracing with the following features:");
print("• Real-time disassembly of generated x64 machine code");
print("• Control flow analysis (branches, jumps, calls)");
print("• Register allocation and usage tracking");
print("• Performance optimization insights");
print("• Assembly instruction categorization");
print("");

// Demo Function 1: Mathematical Hot Loop
// This function is designed to trigger JIT compilation and show optimization
print("--- Demo 1: Mathematical Hot Loop ---");
function mathHotLoop(iterations) {
    var sum = 0;
    var product = 1;

    // Hot loop that should be optimized by JIT
    for (var i = 1; i <= iterations; i++) {
        sum += Math.sqrt(i) * 2.5;
        product = (product * 1.001) % 1000;

        // Some conditional logic to create branches
        if (i % 100 === 0) {
            sum = sum * 0.99;  // Occasional adjustment
        }
    }

    return { sum: sum, product: product, iterations: iterations };
}

var mathResult = mathHotLoop(10000);
print("Math result: sum=" + mathResult.sum.toFixed(2) +
      ", product=" + mathResult.product.toFixed(2));

// Demo Function 2: Complex Object Operations
// Shows object property access optimization and polymorphic inline caching
print("\n--- Demo 2: Object Property Access ---");
function Point(x, y) {
    this.x = x;
    this.y = y;
}

Point.prototype.distance = function(other) {
    var dx = this.x - other.x;
    var dy = this.y - other.y;
    return Math.sqrt(dx * dx + dy * dy);
};

Point.prototype.scale = function(factor) {
    this.x *= factor;
    this.y *= factor;
    return this;
};

function processPoints() {
    var points = [];

    // Create many objects with consistent shape for hidden class optimization
    for (var i = 0; i < 1000; i++) {
        points.push(new Point(Math.random() * 100, Math.random() * 100));
    }

    var totalDistance = 0;
    var origin = new Point(0, 0);

    // Process points - should show optimized property access
    for (var j = 0; j < points.length; j++) {
        var point = points[j];
        point.scale(1.1);  // Method call optimization
        totalDistance += point.distance(origin);  // Property access optimization
    }

    return totalDistance;
}

var objectResult = processPoints();
print("Object processing result: " + objectResult.toFixed(2));

// Demo Function 3: Recursive Functions with Tail Call Optimization
print("\n--- Demo 3: Recursive Function Analysis ---");
function fibonacci(n, a, b) {
    a = a || 0;
    b = b || 1;

    if (n <= 0) return a;
    if (n === 1) return b;

    // Tail recursive call - may be optimized
    return fibonacci(n - 1, b, a + b);
}

function factorialTailRec(n, acc) {
    acc = acc || 1;
    if (n <= 1) return acc;
    return factorialTailRec(n - 1, n * acc);
}

var fib20 = fibonacci(20);
var fact10 = factorialTailRec(10);
print("Fibonacci(20): " + fib20);
print("Factorial(10): " + fact10);

// Demo Function 4: Array Processing and Memory Access Patterns
print("\n--- Demo 4: Array Operations and Memory Access ---");
function arrayProcessing() {
    var data = [];

    // Initialize large array
    for (var i = 0; i < 10000; i++) {
        data[i] = Math.random();
    }

    // Sequential access pattern
    var sum = 0;
    for (var j = 0; j < data.length; j++) {
        sum += data[j];
    }

    // Filter operation
    var filtered = data.filter(function(x) {
        return x > 0.5;
    });

    // Map operation
    var transformed = data.map(function(x) {
        return x * x + Math.sin(x);
    });

    // Reduce operation
    var product = data.reduce(function(acc, val) {
        return acc * (1 + val * 0.01);
    }, 1.0);

    return {
        sum: sum,
        filteredLength: filtered.length,
        transformedSum: transformed.reduce(function(a, b) { return a + b; }, 0),
        product: product
    };
}

var arrayResult = arrayProcessing();
print("Array processing - Sum: " + arrayResult.sum.toFixed(2) +
      ", Filtered: " + arrayResult.filteredLength +
      ", Product: " + arrayResult.product.toFixed(6));

// Demo Function 5: Function Call Optimization and Inlining
print("\n--- Demo 5: Function Call Patterns ---");
function add(a, b) { return a + b; }
function multiply(a, b) { return a * b; }
function divide(a, b) { return b !== 0 ? a / b : 0; }

function complexCalculation(x, y) {
    // Many function calls - should show inlining optimization
    var result = add(multiply(x, y), divide(x, y));
    result = add(result, multiply(result, 0.1));
    result = divide(result, add(result, 1));
    return result;
}

function callPatternDemo() {
    var total = 0;

    // Hot loop with many function calls
    for (var i = 1; i <= 5000; i++) {
        total += complexCalculation(i, i + 1);
    }

    return total;
}

var callResult = callPatternDemo();
print("Function call result: " + callResult.toFixed(4));

// Demo Function 6: Type Specialization Examples
print("\n--- Demo 6: Type Specialization ---");
function typeSpecializationDemo() {
    // Integer arithmetic - should be optimized to native integer ops
    function integerMath() {
        var sum = 0;
        for (var i = 0; i < 1000; i++) {
            sum += i;  // Pure integer operations
        }
        return sum;
    }

    // Floating point arithmetic - should use FPU instructions
    function floatMath() {
        var sum = 0.0;
        for (var i = 0; i < 1000; i++) {
            sum += i * 3.14159 + Math.sin(i);  // Floating point operations
        }
        return sum;
    }

    // String operations - should show string optimization
    function stringOps() {
        var result = "";
        for (var i = 0; i < 100; i++) {
            result += "item" + i + ";";
        }
        return result.length;
    }

    return {
        intSum: integerMath(),
        floatSum: floatMath(),
        stringLen: stringOps()
    };
}

var typeResult = typeSpecializationDemo();
print("Type specialization - Int: " + typeResult.intSum +
      ", Float: " + typeResult.floatSum.toFixed(2) +
      ", String: " + typeResult.stringLen);

// Final Performance Summary
print("\n=== JIT Trace Demo Summary ===");
print("✓ Mathematical hot loops with register optimization");
print("✓ Object property access with hidden class caching");
print("✓ Recursive function tail call analysis");
print("✓ Array processing with memory access patterns");
print("✓ Function call inlining and optimization");
print("✓ Type specialization for integers, floats, and strings");
print("");
print("Expected JIT Assembly Trace Features:");
print("• Function headers showing memory addresses and sizes");
print("• Complete x64 disassembly with Intel/AT&T syntax");
print("• Control flow markers: [C] Call, [B] Branch, [R] Return");
print("• Register usage analysis highlighting hot registers (RAX, RBX, etc.)");
print("• Performance metrics including instruction categorization");
print("• Optimization pattern detection (loops, inlining, specialization)");
print("• Branch density analysis and performance warnings");
print("");
print("To enable tracing:");
print("  export CHAKRA_TRACE_JIT_ASM=1");
print("  ./ch jit_trace_demo.js");
print("");
print("Demo completed - " + (mathResult.iterations + arrayResult.filteredLength + typeResult.intSum) + " operations processed");
