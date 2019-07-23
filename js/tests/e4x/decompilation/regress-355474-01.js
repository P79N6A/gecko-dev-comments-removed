





































var bug = 355474;
var summary = 'Iterating over XML with WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = function () { for each (var z in <y/>) { print(z); } };

expect = 'function () { for each (var z in <y/>) { print(z); } }';
actual = f + '';
compareSource(1, expect, actual);

expect = actual = 'No Hang';
f();
f();

TEST(2, expect, actual);

END();
