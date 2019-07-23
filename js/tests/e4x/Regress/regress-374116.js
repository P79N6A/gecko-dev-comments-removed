





































gTestfile = 'regress-374116.js';

var BUGNUMBER = 374116;
var summary = 'Crash with <a/>.@b[1] = 2;';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

<a/>.@b[1] = 2;

TEST(1, expect, actual);
END();
