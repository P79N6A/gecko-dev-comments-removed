





































gTestfile = 'regress-337226.js';

var BUGNUMBER = 337226;
var summary = 'function::globalfunction';
var actual = 'No Error';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
START(summary);

var s = function::parseInt;

TEST(1, expect, actual);
END();
