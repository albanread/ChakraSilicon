console.log("Hello from ChakraCore!");
console.log("JavaScript engine is working correctly.");

// Test some basic JavaScript features
var x = 5;
var y = 10;
console.log("5 + 10 =", x + y);

// Test array operations
var arr = [1, 2, 3, 4, 5];
console.log("Array:", arr);
console.log("Array length:", arr.length);

// Test object creation
var obj = {
    name: "ChakraCore",
    version: "1.0",
    working: true
};
console.log("Object:", JSON.stringify(obj));

// Test function
function greet(name) {
    return "Hello, " + name + "!";
}
console.log(greet("World"));

console.log("All tests completed successfully!");
