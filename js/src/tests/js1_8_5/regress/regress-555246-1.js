





if (typeof evalcx == 'function') {
    var cx = evalcx("");
    evalcx("function f() { return this; }", cx);
    f = cx.f;
    assertEq(f(), this);

    evalcx("function g() { 'use strict'; return this; }", cx);
    g = cx.g;
    assertEq(g(), undefined);
}

reportCompare(0, 0, "ok");
