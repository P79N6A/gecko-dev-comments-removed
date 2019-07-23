




































gTestfile = 'regress-290499.js';

var summary = "11.1.5 XMLList Initialiser Don't Crash with empty Initializer";
var BUGNUMBER = 290499;
var actual = "No Crash";
var expect = "No Crash";

printBugNumber(BUGNUMBER);
START(summary);

var emptyList = <></>;

TEST(1, expect, actual);

END();
