




































START("11.1.5 XMLList Initialiser");

var bug = 290499;
var summary = "Don't Crash with empty XMLList Initializer";
var actual = "No Crash";
var expect = "No Crash";

printBugNumber (bug);
printStatus (summary);

var emptyList = <></>;

TEST(1, expect, actual);

END();
