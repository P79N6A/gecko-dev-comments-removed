




































var gTestfile = 'regress-368213.js';

var BUGNUMBER = 368213;
var summary = 'Do not crash with group assignment and sharp variable defn';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
(function() { [] = #1=[] });

reportCompare(expect, actual, summary);
