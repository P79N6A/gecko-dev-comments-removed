

var x;
function check(code) {
    var s = "no exception thrown";
    try {
        eval(code);
    } catch (exc) {
        s = exc.message;
    }

    assertEq(s, "(intermediate value)(...)[Symbol.iterator] is not a function");
}

x = {};
check("for (var v of x) throw fit;");
check("[...x]");
check("Math.hypot(...x)");

x[Symbol.iterator] = "potato";
check("for (var v of x) throw fit;");

x[Symbol.iterator] = {};
check("for (var v of x) throw fit;");

if (typeof reportCompare === "function")
    reportCompare(0, 0, "ok");
