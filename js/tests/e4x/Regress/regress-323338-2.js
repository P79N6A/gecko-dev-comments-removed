





































gTestfile = 'regress-323338-2.js';

var summary = "Do not crash when qn->uri is null";
var BUGNUMBER = 323338;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

<x/>.(function::children());

TEST(1, expect, actual);
END();
