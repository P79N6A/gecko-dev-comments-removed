





































START("Do not crash when qn->uri is null");

var bug = 323338;
var summary = 'Do not crash when qn->uri is null';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

<x/>.(function::children());

TEST(1, expect, actual);
END();
