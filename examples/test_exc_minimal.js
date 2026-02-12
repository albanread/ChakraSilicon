// Minimal exception test - just interpreter first
function test1() {
    try {
        throw "hello";
    } catch(e) {
        print("PASS: " + e);
    }
}

test1();
print("test1 done");

// Cross-function - this is the one that crashed
function thrower() {
    throw "from thrower";
}

function test2() {
    try {
        thrower();
    } catch(e) {
        print("PASS: " + e);
    }
}

test2();
print("test2 done");
print("All done");
