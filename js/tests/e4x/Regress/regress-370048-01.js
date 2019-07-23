





































var bug = 370048;
var summary = 'with (obj) function:: with xml lists';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var list = <></>;

expect = list.function::length;
with (list)
    actual = function::length;

if (expect !== actual)
  throw "function::length does not work under the with statement with an empty xml list";

TEST(1, expect, actual);
END();
