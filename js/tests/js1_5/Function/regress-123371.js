





































var bug = 123371;
var summary = 'Do not crash when newline separates function name from arglist';
var actual = 'No Crash';
var expect = 'No Crash';


printBugNumber (bug);
printStatus (summary);
  
printStatus
('function call succeeded');

reportCompare(expect, actual, summary);
