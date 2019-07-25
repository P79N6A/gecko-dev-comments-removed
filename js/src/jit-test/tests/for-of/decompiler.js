

function tokens(code) {
    var arr = [];
    var s = code.replace(/\w+|[^\s]/g, function (tok) { arr.push(tok); return ""; });
    assertEq(s.trim(), "", "tokens() should find all tokens in code: " + uneval(code));
    return arr;
}

function test(code) {
    var before = "function f() { " + code + " }";
    var after = eval("(" + before + ")").toString();
    assertEq(tokens(before).join(" "), tokens(after).join(" "), "decompiler failed to round-trip");
}


test("for (a of b) { f(a); }");
test("for (a of b) { f(a); g(a); }");


test("for (a of b in c ? c : c.items()) { f(a); }");


test("for ([a, b] of c) { a.m(b); }");


test("for (let a of b) { f(a); }");
test("for (let [a, b] of c) { a.m(b); }");


test("return [a for (a of b)];");
test("return [[b, a] for ([a, b] of c.items())];");


test("return (a for (a of b));");

