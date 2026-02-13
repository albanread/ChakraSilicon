// Unicode output test for ChakraSilicon
// Verifies that the engine and terminal handle UTF-8/Unicode output correctly

function printSection(title, content) {
    console.log("----------------------------------------");
    console.log(title);
    console.log(content);
}

printSection("ASCII", "Hello World");
printSection("Latin-1 Extended", "Ça va? Naïve. Über. Español.");
printSection("General Symbols", "© ® ™ € £ ¥");
printSection("Greek", "α β γ δ ε ζ η θ");
printSection("Cyrillic", "Привет мир (Hello World)");
printSection("East Asian (CJK)", "中文 (Chinese) / 日本語 (Japanese) / 한국어 (Korean)");
printSection("Emojis", "🚀 🍎 💻 🐧 🔥 ✨");

// Test string length and code points for surrogate pairs
const rocket = "🚀";
printSection("Surrogate Pairs Analysis",
    `Character: ${rocket}\n` +
    `Length: ${rocket.length} (Expected: 2)\n` +
    `Code Point: 0x${rocket.codePointAt(0).toString(16).toUpperCase()} (Expected: 0x1F680)`
);

if (rocket.codePointAt(0) === 0x1F680) {
    console.log("\n✅ PASS: Unicode internal representation seems correct.");
} else {
    console.log("\n❌ FAIL: Unicode internal representation incorrect.");
}
