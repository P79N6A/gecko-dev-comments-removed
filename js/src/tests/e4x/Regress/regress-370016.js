





































gTestfile = 'regress-370016.js';

var BUGNUMBER = 370016;
var summary = 'with (nonxmlobj) function::';
var actual = 'No Exception';
var expect = 'No Exception';

printBugNumber(BUGNUMBER);
START(summary);

with (Math) print(function::sin(0))

TEST(1, expect, actual);
END();
