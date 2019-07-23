





assertEq(typeof evalcx, "function", "")
var cx = evalcx("");
evalcx("function f() { return this; }", cx);
f = cx.f;
assertEq(f(), cx);
reportCompare(0, 0, "");
