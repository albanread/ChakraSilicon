// Simple test script for ChakraCore
print('Hello from ChakraCore!');
print('Testing basic functionality...');

// Test arithmetic
var result = 5 + 7;
print('5 + 7 = ' + result);

// Test function
function greet(name) {
    return 'Hello, ' + name + '!';
}

print(greet('Apple Silicon'));

// Test object
var obj = { message: 'ChakraCore is working!' };
print(obj.message);

// Test array
var arr = [1, 2, 3];
arr.push(4);
print('Array length: ' + arr.length);

print('All tests completed successfully!');
