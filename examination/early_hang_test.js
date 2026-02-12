print("Starting test...");

function add(a, b) {
    return a + b;
}

print("Calling add once (interpreted)...");
var result = add(1, 2);
print("Result: " + result);

print("Calling add again (still interpreted)...");
result = add(3, 4);
print("Result: " + result);

print("Test complete!");
