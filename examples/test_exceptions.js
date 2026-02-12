// Test DWARF exception handling - try/catch/finally support
// Tests that work within a single function scope (cross-function throw
// is a separate pre-existing issue on ARM64 macOS)

function testBasicTryCatch() {
    try {
        throw "hello";
    } catch (e) {
        if (e === "hello") {
            print("PASS: basic try/catch");
            return true;
        }
    }
    print("FAIL: basic try/catch");
    return false;
}

function testTryCatchFinally() {
    var finallyRan = false;
    try {
        throw "error";
    } catch (e) {
        if (e !== "error") {
            print("FAIL: try/catch/finally - wrong exception");
            return false;
        }
    } finally {
        finallyRan = true;
    }
    if (finallyRan) {
        print("PASS: try/catch/finally");
        return true;
    }
    print("FAIL: try/catch/finally - finally didn't run");
    return false;
}

function testNestedTryCatch() {
    try {
        try {
            throw "inner";
        } catch (e) {
            if (e !== "inner") {
                print("FAIL: nested - wrong inner exception");
                return false;
            }
            throw "outer";
        }
    } catch (e) {
        if (e === "outer") {
            print("PASS: nested try/catch");
            return true;
        }
    }
    print("FAIL: nested try/catch");
    return false;
}

function testFinallyWithoutThrow() {
    var finallyRan = false;
    try {
        var x = 42;
    } finally {
        finallyRan = true;
    }
    if (finallyRan) {
        print("PASS: finally without throw");
        return true;
    }
    print("FAIL: finally without throw");
    return false;
}

function testTryCatchInLoop() {
    var caught = 0;
    for (var i = 0; i < 5; i++) {
        try {
            if (i % 2 === 0) {
                throw i;
            }
        } catch (e) {
            caught++;
        }
    }
    if (caught === 3) {
        print("PASS: try/catch in loop");
        return true;
    }
    print("FAIL: try/catch in loop (caught " + caught + ", expected 3)");
    return false;
}

function testExceptionObject() {
    try {
        var obj = { message: "test error", code: 42 };
        throw obj;
    } catch (e) {
        if (e.message === "test error" && e.code === 42) {
            print("PASS: exception object");
            return true;
        }
    }
    print("FAIL: exception object");
    return false;
}

function testTryCatchWithComputation() {
    var result = 0;
    for (var i = 0; i < 10; i++) {
        try {
            result += i;
            if (i === 7) {
                throw "stop";
            }
        } catch(e) {
            result *= 2;
        }
    }
    // 0+1+2+3+4+5+6+7 = 28, *2 = 56, +8+9 = 73
    if (result === 73) {
        print("PASS: try/catch with computation");
        return true;
    }
    print("FAIL: try/catch with computation (got " + result + ", expected 73)");
    return false;
}

// Force JIT by calling functions multiple times
function warmUp(fn) {
    for (var i = 0; i < 100; i++) {
        fn();
    }
}

print("=== DWARF Exception Handling Tests (Interpreter) ===");

testBasicTryCatch();
testTryCatchFinally();
testNestedTryCatch();
testFinallyWithoutThrow();
testTryCatchInLoop();
testExceptionObject();
testTryCatchWithComputation();

print("");
print("=== Warming up for JIT ===");
warmUp(testBasicTryCatch);
warmUp(testTryCatchFinally);
warmUp(testNestedTryCatch);
warmUp(testFinallyWithoutThrow);
warmUp(testTryCatchInLoop);
warmUp(testExceptionObject);
warmUp(testTryCatchWithComputation);

print("");
print("=== JIT'd Exception Handling Tests ===");
testBasicTryCatch();
testTryCatchFinally();
testNestedTryCatch();
testFinallyWithoutThrow();
testTryCatchInLoop();
testExceptionObject();
testTryCatchWithComputation();

print("");
print("=== All exception handling tests complete ===");
