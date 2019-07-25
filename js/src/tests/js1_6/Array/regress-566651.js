







var BUGNUMBER = 566651;
var summary = 'setting array.length to null should not throw an uncatchable exception';
var actual = 0;
var expect = 0;

printBugNumber(BUGNUMBER);
printStatus (summary);

var a = [];
a.length = null;

reportCompare(expect, actual, summary);
