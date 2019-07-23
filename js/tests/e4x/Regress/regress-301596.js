





































gTestfile = 'regress-301596.js';

var summary = "E4X - Do not crash with XMLList filters";
var BUGNUMBER = 301596;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    <xml/>.(@a == 1);
    throw 5;
}
catch (e)
{
}

TEST(1, expect, actual);

END();
