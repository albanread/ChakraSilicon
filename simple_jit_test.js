// Simple JIT test to see complete assembly trace
function simpleAdd(a, b) {
    return a + b;
}

// Call it enough times to trigger JIT compilation
print('Starting simple JIT test...');
var result = 0;
for (var i = 0; i < 100; i++) {
    result = simpleAdd(i, i * 2);
}
print('Final result: ' + result);
print('JIT test completed!');
