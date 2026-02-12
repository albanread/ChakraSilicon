// Test script for ChakraCore interpreter-only mode
print("=== ChakraCore Interpreter Test ===");

// Basic arithmetic and variables
var a = 42;
var b = 3.14;
print("Basic math: " + a + " + " + b + " = " + (a + b));

// Arrays and higher-order functions
var numbers = [1, 2, 3, 4, 5, 10];
var sum = numbers.reduce(function(acc, val) { return acc + val; }, 0);
var doubled = numbers.map(function(x) { return x * 2; });
print("Array sum: " + sum);
print("Doubled array: [" + doubled.join(", ") + "]");

// Objects and property access
var person = {
    name: "ChakraCore",
    version: "1.13.0",
    features: ["ES6", "Interpreter", "JIT"],
    greet: function() {
        return "Hello from " + this.name + " " + this.version;
    }
};
print(person.greet());
print("Features: " + person.features.join(", "));

// String manipulation
var message = "JavaScript execution without JIT!";
print("Message length: " + message.length);
print("Uppercase: " + message.toUpperCase());
print("Contains 'JIT': " + message.includes("JIT"));

// Control flow
print("\nCounting down:");
for (var i = 5; i >= 1; i--) {
    print("  " + i);
}
print("  Blast off! 🚀");

// Functions and closures
function createCounter(start) {
    var count = start || 0;
    return function() {
        return ++count;
    };
}
var counter = createCounter(10);
print("\nCounter demo:");
print("Count: " + counter()); // 11
print("Count: " + counter()); // 12
print("Count: " + counter()); // 13

// ES6 features (if supported in interpreter)
try {
    // Let and const
    let es6Message = "ES6 features work!";
    const PI = 3.14159;

    // Template literals
    let templateTest = `Pi is approximately ${PI}`;
    print(templateTest);

    // Arrow functions
    let square = x => x * x;
    print("Square of 7: " + square(7));

    // Destructuring
    let [first, second, ...rest] = [1, 2, 3, 4, 5];
    print("Destructuring: first=" + first + ", second=" + second + ", rest=[" + rest.join(",") + "]");

} catch (e) {
    print("Some ES6 features not supported: " + e.message);
}

// Error handling
try {
    throw new Error("Test error handling");
} catch (e) {
    print("Caught error: " + e.message);
} finally {
    print("Finally block executed");
}

// Async simulation (using setTimeout-like behavior with simple delay)
function delay(ms, callback) {
    // Simple busy wait for demonstration
    var start = Date.now();
    while (Date.now() - start < ms) {
        // Busy wait
    }
    callback();
}

print("\nTesting callback-style async:");
delay(10, function() {
    print("Delayed execution completed!");
});

// Mathematical operations
print("\nMath operations:");
print("sqrt(16) = " + Math.sqrt(16));
print("PI = " + Math.PI);
print("Random: " + Math.random());

// Date operations
var now = new Date();
print("Current time: " + now.toString());

// Regular expressions
var pattern = /test/i;
var testString = "This is a TEST string";
print("Regex test: '" + testString + "' matches /test/i: " + pattern.test(testString));

// JSON operations
var obj = { name: "ChakraCore", type: "JavaScript Engine", interpreter: true };
var jsonString = JSON.stringify(obj);
var parsed = JSON.parse(jsonString);
print("JSON round-trip: " + parsed.name + " (" + parsed.type + ")");

print("\n=== All interpreter tests completed successfully! ===");
