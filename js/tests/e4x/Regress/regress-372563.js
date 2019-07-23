





































var bug = 372563;
var summary = 'Assertion failure: ss->top >= 2';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

function() { *() }
TEST(1, expect, actual);
END();
