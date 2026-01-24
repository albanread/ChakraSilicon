// Simple test script for ChakraCore
console.log = print; // ChakraCore uses print() instead of console.log

print("Hello from ChakraCore!");
print("Testing basic JavaScript functionality...");

// Test basic arithmetic
var a = 5;
var b = 10;
var sum = a + b;
print("5 + 10 = " + sum);

// Test arrays
var arr = [1, 2, 3, 4, 5];
print("Array: " + arr.join(", "));

// Test objects
var person = {
    name: "John",
    age: 30,
    city: "New York"
};
print("Person: " + person.name + ", Age: " + person.age + ", City: " + person.city);

// Test functions
function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

print("Fibonacci sequence:");
for (var i = 0; i < 10; i++) {
    print("fib(" + i + ") = " + fibonacci(i));
}

// Test ES6 features (if supported)
try {
    let message = "ES6 let/const works!";
    const PI = 3.14159;
    print(message);
    print("PI = " + PI);

    // Arrow function
    var square = (x) => x * x;
    print("Square of 7: " + square(7));

    // Template literals
    var name = "ChakraCore";
    var greeting = `Hello, ${name}!`;
    print(greeting);
} catch (e) {
    print("ES6 features not fully supported: " + e.message);
}

print("Test completed successfully!");
