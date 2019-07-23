





































gTestfile = 'regress-374160.js';

var BUGNUMBER = 374160;
var summary = 'Do not assert with <a><b c="1"></b><b c="2"></b></a>..@c[0]=3';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

<a><b c="1"></b><b c="2"></b></a>..@c[0] = 3;
TEST(1, expect, actual);

END();
