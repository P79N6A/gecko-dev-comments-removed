




































var bug = 317714;
var summary = 'Regression test for regression from bug 316885';
var actual = 'No Crash';
var expect = 'No Crash';

var d5="-1";
var r3=d5.split(":");
r3[0]++;

printBugNumber (bug);
printStatus (summary);

reportCompare(expect, actual, summary);
