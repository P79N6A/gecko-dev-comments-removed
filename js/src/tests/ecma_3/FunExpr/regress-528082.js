




var BUGNUMBER = 528082;
var summary = 'named function expression function-name-as-upvar slot botch';

printBugNumber(BUGNUMBER);
printStatus(summary);

function f() {
    return function g(a) { return function () { return g; }(); }();
}
var actual = typeof f();
var expect = "function";

reportCompare(expect, actual, summary);

printStatus("All tests passed!");
