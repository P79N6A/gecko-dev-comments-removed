





































gTestfile = 'regress-355474-01.js';

var BUGNUMBER = 355474;
var summary = 'Iterating over XML with WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = function () { for each (var z in <y/>) { print(z); } };

expect = 'function () { for each (var z in <y/>) { print(z); } }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

expect = actual = 'No Hang';
f();
f();

TEST(2, expect, actual);

END();
