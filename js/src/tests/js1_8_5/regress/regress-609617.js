





var actual;
var expect = "pass";

var x = "fail";
function f() {
    var x = "pass"; 
    delete(eval("actual = x"));
}
f();
assertEq(actual, expect);

function g() { return 1 }
function h() { function g() { throw 2; } eval('g()')++; } 

try {
    h();
    assertEq(0, -1);
} catch (e) {
    assertEq(e, 2);
}

var lhs_prefix = ["",        "++", "--", "",   "",   "[",             "[y, "      ];
var lhs_suffix = [" = 'no'", "",   "",   "++", "--", ", y] = [3, 4]", "] = [5, 6]"];

for (var i = 0; i < lhs_prefix.length; i++) {
    try {
        eval(lhs_prefix[i] + "eval('x')" + lhs_suffix[i]);
        assertEq(i, -2);
    } catch (e) {
        if (/\[/.test(lhs_prefix[i])) {
            assertEq(e.message, "invalid destructuring target");
        } else {
            




            assertEq(e.message, "invalid assignment left-hand side");
        }
    }
}


for (var i = 0; i < lhs_prefix.length; i++) {
    try {
        eval("(function () { 'use strict'; " + lhs_prefix[i] + "foo('x')" + lhs_suffix[i] + "; })");
        assertEq(i, -3);
    } catch (e) {
        if (/\+\+|\-\-/.test(lhs_prefix[i] || lhs_suffix[i]))
            assertEq(e.message, "invalid increment/decrement operand");
        else if (/\[/.test(lhs_prefix[i]))
            assertEq(e.message, "invalid destructuring target");
        else
            assertEq(e.message, "invalid assignment left-hand side");
    }
}





var fooArg;
function foo(arg) { fooArg = arg; }
try {
    eval("delete (foo('x') = 42);");
    assertEq(0, -4);
} catch (e) {
    assertEq(e.message, "invalid assignment left-hand side");
}
assertEq(fooArg, 'x');


function g() {
    "use strict";
    assertEq(delete Object(), true);
}
g();

reportCompare(0, 0, "ok");
