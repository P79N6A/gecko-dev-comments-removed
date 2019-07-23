





































gTestfile = 'regress-373595-03.js';

var summary = '13.3 QName Objects - Do not assert: op == JSOP_ADD';
var BUGNUMBER = 373595;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

print('This testcase does not reproduce the original issue');

"" + (function () { x.@foo < 1;});

TEST(1, expect, actual);

END();
