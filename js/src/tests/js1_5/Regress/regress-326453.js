






var gTestfile = 'regress-326453.js';

var BUGNUMBER = 326453;
var summary = 'Do not assert: while decompiling';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f() { with({})function g() { }; printStatus(); }

printStatus(f.toString());

reportCompare(expect, actual, summary);
