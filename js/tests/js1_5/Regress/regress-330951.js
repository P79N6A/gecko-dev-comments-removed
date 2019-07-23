




































var gTestfile = 'regress-330951.js';

var BUGNUMBER = 330951;
var summary = 'Crash in Array.sort on array with undefined value';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

[undefined,1].sort();
 
reportCompare(expect, actual, summary);
