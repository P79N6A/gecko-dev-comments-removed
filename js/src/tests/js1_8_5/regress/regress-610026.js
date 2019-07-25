





var expect = "pass";
var actual;





var s = "{}";
for (var i = 0; i < 19; i++)
    s += s;

try {
    eval(s);
    actual = "pass";
} catch (e) {
    actual = "fail: " + e;
}

assertEq(actual, expect);

s += s.slice(0, -2);

try {
    eval(s);
    actual = "pass";
} catch (e) {
    actual = "fail: " + e;
}

assertEq(actual, expect);

s += "{}";

try {
    eval(s);
    actual = "fail: expected InternalError: program too large";
} catch (e) {
    actual = (e.message == "program too large") ? "pass" : "fail: " + e;
}

assertEq(actual, expect);


s = "{let y, x;}";
for (i = 0; i < 16; i++)
    s += s;


s += "var g; { let x = 42; g = function() { return x; }; x = x; }";

try {
    eval(s);
    actual = g();
} catch (e) {
    actual = e;
}
assertEq(actual, 42);

reportCompare(0, 0, "ok");
