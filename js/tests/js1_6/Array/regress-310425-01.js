




































var gTestfile = 'regress-310425-01.js';

var BUGNUMBER = 310425;
var summary = 'Array.indexOf/lastIndexOf edge cases';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = -1;
actual = [].lastIndexOf(undefined, -1);
reportCompare(expect, actual, summary);

expect = -1;
actual = [].indexOf(undefined, -1);
reportCompare(expect, actual, summary);

var x = [];
x[1 << 30] = 1;
expect = (1 << 30);
actual = x.lastIndexOf(1);
reportCompare(expect, actual, summary);
