





































var gTestfile = 'regress-123371.js';

var BUGNUMBER = 123371;
var summary = 'Do not crash when newline separates function name from arglist';
var actual = 'No Crash';
var expect = 'No Crash';


printBugNumber(BUGNUMBER);
printStatus (summary);
 
printStatus
('function call succeeded');

reportCompare(expect, actual, summary);
