






































gTestfile = 'regress-410192.js';

var summary = 'Proper quoting of attribute by uneval/toSource';
var BUGNUMBER = 410192;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = '"v"';
actual = uneval(<x a="v"/>.@a);

TEST(1, expect, actual);

END();
