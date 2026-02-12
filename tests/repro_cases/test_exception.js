// test_exception.js — Test exception handling in various modes
// Run with:
//   -NoNative   → interpreter only
//   -off:FullJit → SimpleJit + interpreter  
//   (default)   → FullJit + SimpleJit + interpreter

function throwingFunction() {
    throw new Error("test exception");
}

function testBasicTryCatch() {
    try {
        throwingFunction();
        print("FAIL: should not reach here");
    } catch (e) {
        print("PASS: caught: " + e.message);
    }
}

function testNestedTryCatch() {
    try {
        try {
            throwingFunction();
            print("FAIL: inner should not reach here");
        } catch (e) {
            print("PASS: inner caught: " + e.message);
            throw new Error("rethrown");
        }
        print("FAIL: outer should not reach here");
    } catch (e) {
        print("PASS: outer caught: " + e.message);
    }
}

function testFinally() {
    var finallyRan = false;
    try {
        throwingFunction();
    } catch (e) {
        print("PASS: caught in finally test: " + e.message);
    } finally {
        finallyRan = true;
    }
    if (finallyRan) {
        print("PASS: finally ran");
    } else {
        print("FAIL: finally did not run");
    }
}

function testTryCatchInLoop() {
    var caught = 0;
    for (var i = 0; i < 100; i++) {
        try {
            throwingFunction();
        } catch (e) {
            caught++;
        }
    }
    if (caught === 100) {
        print("PASS: caught all 100 exceptions in loop");
    } else {
        print("FAIL: only caught " + caught + " of 100");
    }
}

print("=== Exception handling tests ===");
testBasicTryCatch();
testNestedTryCatch();
testFinally();
testTryCatchInLoop();
print("=== All tests complete ===");
