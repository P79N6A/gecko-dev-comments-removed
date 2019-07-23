




































var bug = 368213;
var summary = 'Do not crash with group assignment and sharp variable defn';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);
  
function() { [] = #1=[] }

reportCompare(expect, actual, summary);
