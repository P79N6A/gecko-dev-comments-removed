





































START("E4X - Should not repress exceptions");

var bug = 301553;
var summary = 'Throw exceptions';
var actual = 'No exception';
var expect = 'exception';

printBugNumber (bug);
printStatus (summary);

try
{
    var x = <xml/>;
    Object.toString.call(x);
}
catch(e)
{
    actual = 'exception';
}

TEST(1, expect, actual);
END();
