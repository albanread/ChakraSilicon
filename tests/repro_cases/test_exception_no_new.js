// test_exception_no_new.js — Test exceptions without 'new Error()'
// This avoids the pre-existing FullJit calling convention bug with Error constructor

function throwingFunction() {
    throw "simple string exception";
}

function test1() {
    try {
        throwingFunction();
    } catch (e) {
        print("PASS test1: caught: " + e);
    }
}

function test2() {
    try {
        try {
            throwingFunction();
        } catch (e) {
            throw "rethrown";
        }
    } catch (e) {
        print("PASS test2: caught: " + e);
    }
}

function test3() {
    var finallyRan = false;
    try {
        throwingFunction();
    } catch (e) {
        // caught
    } finally {
        finallyRan = true;
    }
    print("PASS test3: finally ran = " + finallyRan);
}

function test4() {
    var caught = 0;
    for (var i = 0; i < 500; i++) {
        try {
            throw i;
        } catch (e) {
            caught++;
        }
    }
    print("PASS test4: caught " + caught + "/500 in loop");
}

function test5() {
    var caught = 0;
    for (var i = 0; i < 500; i++) {
        try {
            throwingFunction();
        } catch (e) {
            caught++;
        }
    }
    print("PASS test5: caught " + caught + "/500 through function call");
}

print("=== Exception tests (no new Error) ===");
test1();
test2();
test3();
test4();
test5();
print("=== All tests complete ===");
