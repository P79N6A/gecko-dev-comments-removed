





































START("Do not define AnyName");

var bug = 322499;
var summary = 'Do not define AnyName';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

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
