





function f(y) {
    eval("let (z=2, w=y) { (function () { w.p = 7; })(); }");
}
var x = {};
f(x);
assertEq(x.p, 7);
print(" PASSED!");
