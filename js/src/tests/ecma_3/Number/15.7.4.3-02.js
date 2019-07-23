




































var gTestfile = '15.7.4.3-02.js';

var BUGNUMBER = "446494";
var summary = "num.toLocaleString should handle exponents";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);

expect = '1e-10';
actual = 1e-10.toLocaleString();
reportCompare(expect, actual, summary + ': ' + expect);

expect = 'Infinity';
actual = Infinity.toLocaleString();
reportCompare(expect, actual, summary + ': ' + expect);
