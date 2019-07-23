





































gTestfile = 'regress-322499.js';

var summary = "Do not define AnyName";
var BUGNUMBER = 322499;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'ReferenceError';

try
{
    AnyName;
}
catch(ex)
{
    actual = ex.name;
}

TEST(1, expect, actual);

try
{
    *;
}
catch(ex)
{
    actual = ex.name;
}
TEST(2, expect, actual);

END();
