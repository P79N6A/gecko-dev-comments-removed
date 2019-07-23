





































var bug = 372564;
var summary = 'Assertion failure: op == JSOP_ADD';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

function() { return {a: @foo} <= 3;}
TEST(1, expect, actual);
END();
