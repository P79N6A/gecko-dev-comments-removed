





var BUGNUMBER = 368213;
var summary = 'Do not crash with group assignment and sharp variable defn';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
(function() { [] = [] });

reportCompare(expect, actual, summary);
