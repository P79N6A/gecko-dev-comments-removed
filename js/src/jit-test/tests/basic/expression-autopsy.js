load(libdir + "asserts.js");

function check_one(expected, f, err) {
    var failed = true;
    try {
        f();
        failed = false;
    } catch (ex) {
        var s = ex.toString();
        assertEq(s.slice(0, 11), "TypeError: ");
        assertEq(s.slice(-err.length), err, "" + f);
        assertEq(s.slice(11, -err.length), expected);
    }
    if (!failed)
        throw new Error("didn't fail");
}
ieval = eval;
function check(expr, expected=expr) {
    var end, err;
    for ([end, err] of [[".random_prop", " is undefined"], ["()", " is not a function"]]) {
        var statement = "o = {};" + expr + end, f;
        var cases = [
            
            function () {
                ieval("var o, undef;\n" + statement);
            },
            
            Function("o", "undef", statement),
            
            Function("var o, undef;\n" + statement),
            
            Function("arg1", "arg2", "var o, undef;\n" + statement),
            
            Function("o", "undef", "with (Object) {}\n" + statement),
            
            Function("with (Object) { " + statement + " }"),
            
            Function("o", "undef", "function myfunc() { return o + undef; }\n" + statement),
            
            Function("{ let o, undef;\n" + statement + "}"),
            
            Function("let (o, undef) { " + statement + " }"),
            
            Function("var v1, v2; let (o, undef) { " + statement + " }"),
            
            Function("o", "undef", "let (o, undef) { " + statement + " }"),
            
            Function("var x = 4; switch (x) { case 4: let o, undef;" + statement + "\ncase 6: break;}"),
            
            Function("let (x=4, y=5) { x + y; }\nlet (a, b, c) { a + b - c; }\nlet (o, undef) {" + statement + " }"),
            
            Function("o", "undef", "let ([] = []) {} let (o, undef) { " + statement + " }"),
            
            Function("o", "undef", "try { let q = 4; try { let p = 4; } catch (e) {} } catch (e) {} let (o, undef) { " + statement + " }")
        ];

        try {
            
            check_one(expected,
                      Function("var undef, o; for (let z in [1, 2]) { " + statement + " }"),
                      err);
        } catch (ex) {
            
            if (expected == 'undef' && err == ' is undefined')
                check_one(expected + end,
                          Function("var undef, o; for (let z in [1, 2]) { " + statement + " }"),
                          err);
            else
                throw ex;
        }

        for (var f of cases) {
            check_one(expected, f, err);
        }
    }
}

check("undef");
check("o.b");
check("o.length");
check("o[true]");
check("o[false]");
check("o[null]");
check("o[0]");
check("o[1]");
check("o[3]");
check("o[256]");
check("o[65536]");
check("o[268435455]");
check("o['1.1']");
check("o[4 + 'h']", "o['4h']");
check("this.x");
check("ieval(undef)", "ieval(...)");
check("ieval.call()", "ieval.call(...)");
check("ieval(...[])", "ieval(...)");
check("ieval(...[undef])", "ieval(...)");
check("ieval(...[undef, undef])", "ieval(...)");

for (let tok of ["|", "^", "&", "==", "!==", "===", "!==", "<", "<=", ">", ">=",
                 ">>", "<<", ">>>", "+", "-", "*", "/", "%"]) {
    check("o[(undef " + tok + " 4)]");
}

check("o[!(o)]");
check("o[~(o)]");
check("o[+ (o)]");
check("o[- (o)]");


check_one("6", (function () { 6() }), " is not a function");
check_one("Array.prototype.reverse.call(...)", (function () { Array.prototype.reverse.call('123'); }), " is read-only");
check_one("(intermediate value)[(intermediate value)](...).next(...).value", function () { var [{ x }] = [null, {}]; }, " is null");
check_one("(intermediate value)[(intermediate value)](...).next(...).value", function () { ieval("let (x) { var [a, b, [c0, c1]] = [x, x, x]; }") }, " is undefined");


assertThrowsInstanceOf(function () { for (let x of undefined) {} }, TypeError);
