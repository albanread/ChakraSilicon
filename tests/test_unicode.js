// =============================================================================
// Unicode & Internationalization Test Suite for ChakraSilicon
// Tests basic Unicode handling, string operations, and Intl API availability
// =============================================================================

var passed = 0;
var failed = 0;
var errors = [];

function test(name, fn) {
  try {
    var result = fn();
    if (result === true) {
      passed++;
      print("  PASS: " + name);
    } else {
      failed++;
      errors.push(name + " => returned: " + result);
      print("  FAIL: " + name + " => " + result);
    }
  } catch (e) {
    failed++;
    errors.push(name + " => EXCEPTION: " + e.message);
    print("  FAIL: " + name + " => EXCEPTION: " + e.message);
  }
}

// =============================================================================
print("=== 1. Basic Unicode String Literals ===");
// =============================================================================

test("ASCII string length", function () {
  return "hello".length === 5;
});

test("Unicode escape \\u0041 = A", function () {
  return "\u0041" === "A";
});

test("Unicode escape \\u00e9 = é", function () {
  return "\u00e9" === "é";
});

test("Unicode escape \\u4e16 (世)", function () {
  var s = "\u4e16";
  return s === "世" && s.length === 1;
});

test("CJK string 世界 length", function () {
  return "世界".length === 2;
});

test("Cyrillic Привет length", function () {
  return "Привет".length === 6;
});

test("Arabic مرحبا length", function () {
  return "مرحبا".length === 5;
});

test("Korean 안녕하세요 length", function () {
  return "안녕하세요".length === 5;
});

test("Japanese こんにちは length", function () {
  return "こんにちは".length === 5;
});

test("Thai สวัสดี length", function () {
  // Thai combining characters make this longer than it looks
  var s = "สวัสดี";
  return s.length === 6;
});

test("Hindi नमस्ते length", function () {
  var s = "नमस्ते";
  return s.length === 6;
});

// =============================================================================
print("\n=== 2. Surrogate Pairs & Astral Plane (U+10000+) ===");
// =============================================================================

test("Emoji 😀 is surrogate pair (length 2)", function () {
  return "😀".length === 2;
});

test("Emoji charCodeAt returns high surrogate", function () {
  var code = "😀".charCodeAt(0);
  return code === 0xd83d;
});

test("Emoji charCodeAt returns low surrogate", function () {
  var code = "😀".charCodeAt(1);
  return code === 0xde00;
});

test("String.fromCharCode with surrogate pair", function () {
  var s = String.fromCharCode(0xd83d, 0xde00);
  return s === "😀";
});

test("𝄞 (Musical Symbol G Clef U+1D11E) length", function () {
  return "𝄞".length === 2;
});

test("𝐀 (Mathematical Bold Capital A U+1D400) length", function () {
  return "𝐀".length === 2;
});

test("Multiple emoji string length", function () {
  // Each emoji is 2 UTF-16 code units
  return "😀😂🎉".length === 6;
});

// ES6 codePointAt (if supported)
test("codePointAt for emoji", function () {
  if (typeof "".codePointAt !== "function") return "codePointAt not supported";
  return "😀".codePointAt(0) === 0x1f600;
});

test("String.fromCodePoint for emoji", function () {
  if (typeof String.fromCodePoint !== "function")
    return "fromCodePoint not supported";
  return String.fromCodePoint(0x1f600) === "😀";
});

test("String.fromCodePoint for G Clef", function () {
  if (typeof String.fromCodePoint !== "function")
    return "fromCodePoint not supported";
  return String.fromCodePoint(0x1d11e) === "𝄞";
});

// =============================================================================
print("\n=== 3. Unicode String Operations ===");
// =============================================================================

test("indexOf with Unicode", function () {
  return "hello世界foo".indexOf("世界") === 5;
});

test("indexOf with emoji", function () {
  return "abc😀def".indexOf("😀") === 3;
});

test("slice with CJK", function () {
  return "你好世界".slice(1, 3) === "好世";
});

test("substring with CJK", function () {
  return "你好世界".substring(2) === "世界";
});

test("split with Unicode", function () {
  var parts = "a,β,γ,δ".split(",");
  return parts.length === 4 && parts[1] === "β" && parts[3] === "δ";
});

test("replace with Unicode", function () {
  return "hello世界".replace("世界", "world") === "helloworld";
});

test("concat with mixed scripts", function () {
  var s = "Hello" + " " + "世界" + " " + "مرحبا";
  return s === "Hello 世界 مرحبا";
});

test("charAt with Unicode", function () {
  return "café".charAt(3) === "é";
});

test("trim with Unicode content", function () {
  return "  héllo  ".trim() === "héllo";
});

test("repeat with Unicode", function () {
  if (typeof "".repeat !== "function") return "repeat not supported";
  return "あ".repeat(3) === "あああ";
});

test("startsWith with Unicode", function () {
  if (typeof "".startsWith !== "function") return "startsWith not supported";
  return "世界你好".startsWith("世界");
});

test("endsWith with Unicode", function () {
  if (typeof "".endsWith !== "function") return "endsWith not supported";
  return "hello世界".endsWith("世界");
});

test("includes with Unicode", function () {
  if (typeof "".includes !== "function") return "includes not supported";
  return "abc世界def".includes("世界");
});

// =============================================================================
print("\n=== 4. Unicode Case Conversion ===");
// =============================================================================

test("toUpperCase ASCII", function () {
  return "hello".toUpperCase() === "HELLO";
});

test("toLowerCase ASCII", function () {
  return "HELLO".toLowerCase() === "hello";
});

test("toUpperCase Latin extended (café)", function () {
  return "café".toUpperCase() === "CAFÉ";
});

test("toLowerCase Latin extended (CAFÉ)", function () {
  return "CAFÉ".toLowerCase() === "café";
});

test("toUpperCase German ß", function () {
  // ß uppercases to SS in standard Unicode
  var result = "ß".toUpperCase();
  return result === "SS";
});

test("toUpperCase Greek", function () {
  return "αβγδ".toUpperCase() === "ΑΒΓΔ";
});

test("toLowerCase Greek", function () {
  return "ΑΒΓΔ".toLowerCase() === "αβγδ";
});

test("toUpperCase Cyrillic", function () {
  return "привет".toUpperCase() === "ПРИВЕТ";
});

test("CJK unchanged by toUpperCase", function () {
  return "世界".toUpperCase() === "世界";
});

// =============================================================================
print("\n=== 5. Unicode in Regular Expressions ===");
// =============================================================================

test("Regex match CJK characters", function () {
  var m = "hello世界".match(/[\u4e00-\u9fff]+/);
  return m !== null && m[0] === "世界";
});

test("Regex match accented chars", function () {
  var m = "café".match(/caf\u00e9/);
  return m !== null && m[0] === "café";
});

test("Regex replace with Unicode", function () {
  return "foo世界bar".replace(/[\u4e00-\u9fff]+/, "WORLD") === "fooWORLDbar";
});

test("Regex split on Unicode", function () {
  var parts = "a世b界c".split(/[\u4e00-\u9fff]/);
  return (
    parts.length === 3 &&
    parts[0] === "a" &&
    parts[1] === "b" &&
    parts[2] === "c"
  );
});

test("Regex test with Cyrillic", function () {
  return /^[А-Яа-яЁё]+$/.test("Привет");
});

test("Regex global match count", function () {
  // String "aéàâäa" contains 4 accented chars matching the class: é, à, â, ä
  var matches = "aéàâäa".match(/[àâäéèê]/g);
  return matches !== null && matches.length === 4;
});

test("Regex Unicode escape in character class", function () {
  return /[\u0080-\u024f]/.test("ñ");
});

// Unicode flag (u) if supported
test("Regex unicode flag (if supported)", function () {
  try {
    var re = new RegExp(".", "u");
    // With u flag, . should match a full code point
    return true; // If we get here, u flag is supported
  } catch (e) {
    return "u flag not supported: " + e.message;
  }
});

// =============================================================================
print("\n=== 6. Unicode in JSON ===");
// =============================================================================

test("JSON.stringify with Unicode", function () {
  var obj = { name: "世界" };
  var json = JSON.stringify(obj);
  return json === '{"name":"世界"}';
});

test("JSON.parse with Unicode", function () {
  var obj = JSON.parse('{"name":"世界"}');
  return obj.name === "世界";
});

test("JSON.stringify with emoji", function () {
  var obj = { mood: "😀" };
  var json = JSON.stringify(obj);
  var parsed = JSON.parse(json);
  return parsed.mood === "😀";
});

test("JSON with Unicode escapes", function () {
  var obj = JSON.parse('{"val":"\\u0048\\u0065\\u006c\\u006c\\u006f"}');
  return obj.val === "Hello";
});

test("JSON.stringify escapes control characters", function () {
  var json = JSON.stringify("line1\nline2\ttab");
  return json === '"line1\\nline2\\ttab"';
});

test("JSON roundtrip mixed scripts", function () {
  var original = { en: "Hello", zh: "你好", ar: "مرحبا", emoji: "🎉" };
  var roundtripped = JSON.parse(JSON.stringify(original));
  return (
    roundtripped.en === "Hello" &&
    roundtripped.zh === "你好" &&
    roundtripped.ar === "مرحبا" &&
    roundtripped.emoji === "🎉"
  );
});

// =============================================================================
print("\n=== 7. Unicode in Object Keys & Property Access ===");
// =============================================================================

test("Unicode property name", function () {
  var obj = {};
  obj["名前"] = "value";
  return obj["名前"] === "value";
});

test("Unicode property via dot notation (if identifiers allow)", function () {
  // JS allows Unicode letters in identifiers
  try {
    var obj = eval("({ café: 42 })");
    return obj["café"] === 42;
  } catch (e) {
    return "eval failed: " + e.message;
  }
});

test("Emoji as property key", function () {
  var obj = {};
  obj["😀"] = "smile";
  return obj["😀"] === "smile";
});

test("Object.keys with Unicode", function () {
  var obj = { α: 1, β: 2, γ: 3 };
  var keys = Object.keys(obj);
  return keys.length === 3 && keys.indexOf("β") !== -1;
});

test("Map with Unicode keys", function () {
  if (typeof Map !== "function") return "Map not supported";
  var m = new Map();
  m.set("世界", 42);
  m.set("🎉", "party");
  return m.get("世界") === 42 && m.get("🎉") === "party" && m.size === 2;
});

test("Set with Unicode values", function () {
  if (typeof Set !== "function") return "Set not supported";
  var s = new Set(["α", "β", "γ", "α"]);
  return s.size === 3 && s.has("β");
});

// =============================================================================
print("\n=== 8. Unicode Comparison & Sorting ===");
// =============================================================================

test("String equality with identical Unicode", function () {
  return "café" === "caf\u00e9";
});

test("String comparison with < operator", function () {
  // Compared by code point value
  return "a" < "b" && "A" < "a";
});

test("localeCompare exists", function () {
  return typeof "a".localeCompare === "function";
});

test("localeCompare basic", function () {
  var result = "a".localeCompare("b");
  return result < 0;
});

test("Array sort with Unicode strings", function () {
  var arr = ["γ", "α", "β"];
  arr.sort();
  return arr[0] === "α" && arr[1] === "β" && arr[2] === "γ";
});

// =============================================================================
print("\n=== 9. encodeURI / decodeURI with Unicode ===");
// =============================================================================

test("encodeURIComponent with CJK", function () {
  var encoded = encodeURIComponent("世界");
  return encoded === "%E4%B8%96%E7%95%8C";
});

test("decodeURIComponent with CJK", function () {
  return decodeURIComponent("%E4%B8%96%E7%95%8C") === "世界";
});

test("encodeURI roundtrip", function () {
  var original = "hello 世界 café";
  return decodeURI(encodeURI(original)) === original;
});

test("encodeURIComponent with emoji", function () {
  var encoded = encodeURIComponent("😀");
  var decoded = decodeURIComponent(encoded);
  return decoded === "😀";
});

test("encodeURIComponent with Cyrillic", function () {
  var encoded = encodeURIComponent("Привет");
  var decoded = decodeURIComponent(encoded);
  return decoded === "Привет";
});

// =============================================================================
print("\n=== 10. String.prototype.normalize (if supported) ===");
// =============================================================================

test("normalize NFC supported", function () {
  if (typeof "".normalize !== "function") return "normalize not supported";
  // é can be represented as U+00E9 (precomposed) or U+0065 U+0301 (decomposed)
  var precomposed = "\u00e9"; // é
  var decomposed = "\u0065\u0301"; // e + combining accent
  return (
    precomposed !== decomposed &&
    precomposed.length === 1 &&
    decomposed.length === 2
  );
});

test("normalize NFC", function () {
  if (typeof "".normalize !== "function") return "normalize not supported";
  var decomposed = "\u0065\u0301";
  var normalized = decomposed.normalize("NFC");
  return normalized === "\u00e9" && normalized.length === 1;
});

test("normalize NFD", function () {
  if (typeof "".normalize !== "function") return "normalize not supported";
  var precomposed = "\u00e9";
  var normalized = precomposed.normalize("NFD");
  return normalized === "\u0065\u0301" && normalized.length === 2;
});

test("normalize Korean Hangul (NFC)", function () {
  if (typeof "".normalize !== "function") return "normalize not supported";
  // Hangul Jamo: ᄒ (U+1112) + ᅡ (U+1161) + ᆫ (U+11AB) should compose to 한 (U+D55C)
  var jamo = "\u1112\u1161\u11ab";
  var normalized = jamo.normalize("NFC");
  return normalized === "\ud55c";
});

// =============================================================================
print("\n=== 11. Intl API Availability (ICU dependent) ===");
// =============================================================================

test("Intl object exists", function () {
  return typeof Intl === "object" && Intl !== null;
});

test("Intl.Collator exists", function () {
  if (typeof Intl === "undefined") return "Intl not available";
  return typeof Intl.Collator === "function";
});

test("Intl.Collator basic compare", function () {
  if (typeof Intl === "undefined" || typeof Intl.Collator !== "function")
    return "Intl.Collator not available";
  try {
    var collator = new Intl.Collator("en");
    var result = collator.compare("a", "b");
    return result < 0;
  } catch (e) {
    return "Intl.Collator failed: " + e.message;
  }
});

test("Intl.NumberFormat exists", function () {
  if (typeof Intl === "undefined") return "Intl not available";
  return typeof Intl.NumberFormat === "function";
});

test("Intl.NumberFormat basic", function () {
  if (typeof Intl === "undefined" || typeof Intl.NumberFormat !== "function")
    return "Intl.NumberFormat not available";
  try {
    var nf = new Intl.NumberFormat("en-US");
    var result = nf.format(1234567.89);
    return result === "1,234,567.89" || result === "1,234,567.89"; // slight locale variations
  } catch (e) {
    return "Intl.NumberFormat failed: " + e.message;
  }
});

test("Intl.DateTimeFormat exists", function () {
  if (typeof Intl === "undefined") return "Intl not available";
  return typeof Intl.DateTimeFormat === "function";
});

test("Intl.DateTimeFormat basic", function () {
  if (typeof Intl === "undefined" || typeof Intl.DateTimeFormat !== "function")
    return "Intl.DateTimeFormat not available";
  try {
    var dtf = new Intl.DateTimeFormat("en-US");
    var result = dtf.format(new Date(2025, 0, 1));
    // Should produce something like "1/1/2025"
    return typeof result === "string" && result.length > 0;
  } catch (e) {
    return "Intl.DateTimeFormat failed: " + e.message;
  }
});

test("Intl.PluralRules exists", function () {
  if (typeof Intl === "undefined") return "Intl not available";
  return typeof Intl.PluralRules === "function";
});

test("toLocaleLowerCase with Turkish İ", function () {
  try {
    var result = "\u0130".toLocaleLowerCase("tr");
    // Turkish İ (U+0130, capital I with dot above) lowercases to
    // i (U+0069) or i + combining dot above (U+0069 U+0307)
    return result === "\u0069" || result === "\u0069\u0307";
  } catch (e) {
    return "toLocaleLowerCase failed: " + e.message;
  }
});

test("toLocaleUpperCase with Turkish i", function () {
  try {
    var result = "i".toLocaleUpperCase("tr");
    // Turkish i should uppercase to İ (I with dot above)
    return result === "İ" || result === "\u0130" || result === "I"; // fallback if no locale data
  } catch (e) {
    return "toLocaleUpperCase failed: " + e.message;
  }
});

// =============================================================================
print("\n=== 12. Edge Cases & Robustness ===");
// =============================================================================

test("Empty string operations", function () {
  return "".length === 0 && "".toUpperCase() === "" && "".indexOf("x") === -1;
});

test("Null character in string", function () {
  var s = "a\0b";
  return s.length === 3 && s.charCodeAt(1) === 0;
});

test("BOM character U+FEFF", function () {
  var s = "\uFEFF";
  return s.length === 1 && s.charCodeAt(0) === 0xfeff;
});

test("Replacement character U+FFFD", function () {
  var s = "\uFFFD";
  return s.length === 1 && s.charCodeAt(0) === 0xfffd;
});

test("Zero-width joiner U+200D", function () {
  var s = "\u200D";
  return s.length === 1;
});

test("Zero-width non-joiner U+200C", function () {
  var s = "\u200C";
  return s.length === 1;
});

test("Right-to-left mark U+200F", function () {
  var s = "\u200F";
  return s.length === 1 && s.charCodeAt(0) === 0x200f;
});

test("Long Unicode string (10000 chars)", function () {
  var s = "";
  for (var i = 0; i < 10000; i++) {
    s += "あ";
  }
  return s.length === 10000;
});

test("Unicode in eval", function () {
  try {
    var result = eval('"世界"');
    return result === "世界";
  } catch (e) {
    return "eval failed: " + e.message;
  }
});

test("Unicode in Function constructor", function () {
  try {
    var fn = new Function('return "café"');
    return fn() === "café";
  } catch (e) {
    return "Function constructor failed: " + e.message;
  }
});

test("Lone high surrogate", function () {
  // Should not crash - lone surrogates are valid in JS strings
  var s = "\uD800";
  return s.length === 1 && s.charCodeAt(0) === 0xd800;
});

test("Lone low surrogate", function () {
  var s = "\uDC00";
  return s.length === 1 && s.charCodeAt(0) === 0xdc00;
});

test("Reversed surrogate pair (invalid)", function () {
  // Low then high - invalid but should not crash
  var s = "\uDC00\uD800";
  return s.length === 2;
});

// =============================================================================
print("\n=== 13. Symbol.iterator & for-of with Unicode (ES6) ===");
// =============================================================================

test("String iterator iterates code points", function () {
  if (typeof Symbol === "undefined" || typeof Symbol.iterator === "undefined")
    return "Symbol.iterator not supported";
  try {
    var chars = [];
    var str = "A😀B";
    var iter = str[Symbol.iterator]();
    var next;
    while (!(next = iter.next()).done) {
      chars.push(next.value);
    }
    // Should iterate as ["A", "😀", "B"] (3 items, not 4)
    return (
      chars.length === 3 &&
      chars[0] === "A" &&
      chars[1] === "😀" &&
      chars[2] === "B"
    );
  } catch (e) {
    return "iterator failed: " + e.message;
  }
});

test("Array.from string with emoji", function () {
  if (typeof Array.from !== "function") return "Array.from not supported";
  var arr = Array.from("A😀B");
  return arr.length === 3 && arr[1] === "😀";
});

test("Spread operator on Unicode string", function () {
  try {
    var arr = eval('[..."A😀B"]');
    return arr.length === 3 && arr[1] === "😀";
  } catch (e) {
    return "spread failed: " + e.message;
  }
});

// =============================================================================
print("\n=== 14. Template Literals with Unicode (ES6) ===");
// =============================================================================

test("Template literal with Unicode", function () {
  try {
    var result = eval("`hello 世界`");
    return result === "hello 世界";
  } catch (e) {
    return "template literal failed: " + e.message;
  }
});

test("Tagged template with Unicode", function () {
  try {
    var result = eval("(function tag(strings) { return strings[0]; })`café`");
    return result === "café";
  } catch (e) {
    return "tagged template failed: " + e.message;
  }
});

// =============================================================================
// Summary
// =============================================================================

print("\n=============================================");
print("  RESULTS: " + passed + " passed, " + failed + " failed");
print("=============================================");

if (errors.length > 0) {
  print("\nFailed tests:");
  for (var i = 0; i < errors.length; i++) {
    print("  - " + errors[i]);
  }
}

if (failed === 0) {
  print("\nAll Unicode tests passed! ✓");
} else {
  print("\nSome tests failed — check details above.");
}
