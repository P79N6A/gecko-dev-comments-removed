





































gTestfile = 'regress-372564.js';

var BUGNUMBER = 372564;
var summary = 'Do not assert: op == JSOP_ADD';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

(function() { return {a: @foo} <= 3;});

TEST(1, expect, actual);
END();
