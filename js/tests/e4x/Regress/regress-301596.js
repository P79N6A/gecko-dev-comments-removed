





































START("E4X - Do not crash with XMLList filters");

var bug = 301596;
var summary = 'E4X - Do not crash with XMLList filters';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

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
