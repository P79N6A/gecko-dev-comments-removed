





































gTestfile = 'regress-372563.js';

var BUGNUMBER = 372563;
var summary = 'Do not assert: ss->top >= 2';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

(function() { *() });

TEST(1, expect, actual);
END();
