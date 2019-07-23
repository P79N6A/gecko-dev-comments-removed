




































var bug = 317714;
var summary = 'Regression test for regression from bug 316885';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var r3="-1";
r3[0]++;

reportCompare(expect, actual, summary);


