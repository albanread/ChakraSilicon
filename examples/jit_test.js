// JIT test script - designed to trigger JIT compilation
// This script contains loops and function calls that should force JIT compilation

function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

function heavyComputation() {
    var sum = 0;
    for (var i = 0; i < 10000; i++) {
        sum += Math.sin(i) * Math.cos(i);
    }
    return sum;
}

function matrixMultiply(a, b, size) {
    var result = [];
    for (var i = 0; i < size; i++) {
        result[i] = [];
        for (var j = 0; j < size; j++) {
            result[i][j] = 0;
            for (var k = 0; k < size; k++) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return result;
}

function hotLoop() {
    var result = 0;
    for (var i = 0; i < 100000; i++) {
        result += i * 2;
        if (i % 1000 === 0) {
            result = result / 2;
        }
    }
    return result;
}

// This should trigger JIT compilation
print('Starting JIT compilation test...');

// Call fibonacci multiple times to trigger optimization
print('Testing fibonacci...');
for (var i = 0; i < 5; i++) {
    var result = fibonacci(10);
    print('Fibonacci(10) = ' + result);
}

// Heavy computation loop
print('Testing heavy computation...');
for (var i = 0; i < 3; i++) {
    var result = heavyComputation();
    print('Heavy computation result: ' + result.toFixed(4));
}

// Matrix operations
print('Testing matrix multiplication...');
var matrix1 = [[1, 2], [3, 4]];
var matrix2 = [[5, 6], [7, 8]];
for (var i = 0; i < 10; i++) {
    var result = matrixMultiply(matrix1, matrix2, 2);
    if (i === 0) {
        print('Matrix result: [[' + result[0][0] + ',' + result[0][1] + '], [' + result[1][0] + ',' + result[1][1] + ']]');
    }
}

// Hot loop that should definitely get JIT compiled
print('Testing hot loop...');
for (var i = 0; i < 5; i++) {
    var result = hotLoop();
    print('Hot loop result: ' + result);
}

print('JIT test completed!');
