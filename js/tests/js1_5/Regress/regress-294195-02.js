




































var bug = 294195;
var summary = 'Do not crash during String replace when accessing methods on backreferences';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var result = "".replace(/()/, "$1".slice(0,1))

reportCompare(expect, actual, summary);
