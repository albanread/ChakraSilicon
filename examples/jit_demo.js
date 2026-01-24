// JIT Assembly Tracing Demo for ChakraCore
// This script demonstrates the clean JIT assembly output

function addNumbers(a, b) {
    return a + b + 1;
}

function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

function simpleLoop(count) {
    var sum = 0;
    for (var i = 0; i < count; i++) {
        sum += i * 2;
    }
    return sum;
}

print('=== ChakraCore JIT Tracing Demo ===');
print('This demo shows clean JIT assembly output');
print('');

print('1. Testing simple arithmetic function...');
for (var i = 0; i < 20; i++) {
    addNumbers(i, i + 5);
}
print('   addNumbers() should trigger JIT compilation');

print('');
print('2. Testing recursive function...');
for (var i = 0; i < 10; i++) {
    var result = factorial(5);
    if (i === 0) print('   factorial(5) = ' + result);
}
print('   factorial() should trigger JIT compilation');

print('');
print('3. Testing loop-heavy function...');
for (var i = 0; i < 15; i++) {
    var result = simpleLoop(100);
    if (i === 0) print('   simpleLoop(100) = ' + result);
}
print('   simpleLoop() should trigger JIT compilation');

print('');
print('Demo completed! Check above for JIT assembly traces.');
print('Run with: CHAKRA_TRACE_JIT_ASM=1 ch jit_demo.js');
