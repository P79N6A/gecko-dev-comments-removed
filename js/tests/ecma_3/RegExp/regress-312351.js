




































var gTestfile = 'regress-312351.js';

var BUGNUMBER = 312351;
var summary = 'Do not crash on RegExp(null)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var x = RegExp(null);
 
reportCompare(expect, actual, summary);
