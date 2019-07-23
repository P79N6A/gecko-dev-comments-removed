




































var bug = 312351;
var summary = 'Do not crash on RegExp(null)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var x = RegExp(null);
  
reportCompare(expect, actual, summary);
