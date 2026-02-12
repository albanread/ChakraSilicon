// test_exception_comprehensive.js — Comprehensive exception handling tests
// Tests exception handling across interpreter/JIT boundaries

// Force JIT by calling many times
function hotThrow(msg) {
    throw new Error(msg);
}

// Call hotThrow many times so it gets JIT'd
function warmUp() {
    for (var i = 0; i < 200; i++) {
        try {
            hotThrow("warmup " + i);
        } catch (e) {
            // expected
        }
    }
    print("PASS: warmup complete (200 exceptions caught through JIT'd code)");
}

// Test 1: Basic JIT exception
function test1() {
    try {
        hotThrow("test1");
    } catch (e) {
        print("PASS test1: " + e.message);
    }
}

// Test 2: Exception in deeply nested calls
function deepCall(n) {
    if (n === 0) throw new Error("deep exception");
    return deepCall(n - 1);
}

function test2() {
    try {
        deepCall(50);
    } catch (e) {
        print("PASS test2: caught deep exception at depth 50");
    }
}

// Test 3: Exception with finally blocks
function test3() {
    var order = [];
    try {
        try {
            order.push("before throw");
            hotThrow("test3");
            order.push("FAIL: after throw");
        } finally {
            order.push("inner finally");
        }
    } catch (e) {
        order.push("catch: " + e.message);
    } finally {
        order.push("outer finally");
    }
    var expected = "before throw,inner finally,catch: test3,outer finally";
    if (order.join(",") === expected) {
        print("PASS test3: finally ordering correct");
    } else {
        print("FAIL test3: got '" + order.join(",") + "' expected '" + expected + "'");
    }
}

// Test 4: Exception in constructor
function MyError(msg) {
    this.message = msg;
}

function test4() {
    try {
        throw new MyError("custom error");
    } catch (e) {
        if (e.message === "custom error") {
            print("PASS test4: custom error caught");
        } else {
            print("FAIL test4: wrong message: " + e.message);
        }
    }
}

// Test 5: Re-throw
function test5() {
    var caught = false;
    try {
        try {
            hotThrow("rethrow me");
        } catch (e) {
            throw e;
        }
    } catch (e) {
        if (e.message === "rethrow me") {
            caught = true;
            print("PASS test5: rethrow caught correctly");
        }
    }
    if (!caught) print("FAIL test5: exception not caught");
}

// Test 6: Exception types
function test6() {
    var tests = [
        function() { throw new TypeError("type"); },
        function() { throw new RangeError("range"); },
        function() { throw new ReferenceError("ref"); },
        function() { throw new SyntaxError("syntax"); },
        function() { throw new URIError("uri"); },
        function() { throw 42; },
        function() { throw "string error"; },
        function() { throw null; },
        function() { throw undefined; },
    ];

    var passed = 0;
    for (var i = 0; i < tests.length; i++) {
        try {
            tests[i]();
        } catch (e) {
            passed++;
        }
    }
    if (passed === tests.length) {
        print("PASS test6: all " + passed + " exception types caught");
    } else {
        print("FAIL test6: only " + passed + "/" + tests.length + " caught");
    }
}

// Test 7: Exception in eval
function test7() {
    try {
        eval("throw new Error('eval exception')");
    } catch (e) {
        if (e.message === "eval exception") {
            print("PASS test7: eval exception caught");
        } else {
            print("FAIL test7: wrong message");
        }
    }
}

// Test 8: Try/catch in a loop with JIT
function test8() {
    var sum = 0;
    for (var i = 0; i < 1000; i++) {
        try {
            if (i % 3 === 0) {
                throw i;
            }
            sum += i;
        } catch (e) {
            sum += e * 2;
        }
    }
    // Verify the sum is correct
    var expected = 0;
    for (var i = 0; i < 1000; i++) {
        if (i % 3 === 0) {
            expected += i * 2;
        } else {
            expected += i;
        }
    }
    if (sum === expected) {
        print("PASS test8: loop try/catch sum correct (" + sum + ")");
    } else {
        print("FAIL test8: expected " + expected + " got " + sum);
    }
}

print("=== Comprehensive exception tests ===");
warmUp();
test1();
test2();
test3();
test4();
test5();
test6();
test7();
test8();
print("=== All comprehensive tests complete ===");
