




































var bug = 330951;
var summary = 'Crash in Array.sort on array with undefined value';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

[undefined,1].sort();
  
reportCompare(expect, actual, summary);
