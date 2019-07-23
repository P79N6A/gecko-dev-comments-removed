




































var gTestfile = 'regress-311515.js';

var BUGNUMBER = 311515;
var summary = 'Array.sort should skip holes and undefined during sort';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var a = [, 1, , 2, undefined];

actual = a.sort().toString();
expect = '1,2,,,'; 

reportCompare(expect, actual, summary);
