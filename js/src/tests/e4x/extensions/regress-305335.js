





































var summary = "Regression - XML instance methods should type check in " +
    "JS_GetPrivate()";
var BUGNUMBER = 305335;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var o = new Number(0);
try
{
    o.__proto__ = XML();
}
catch (e)
{
    assertEq(e instanceof TypeError, true);
}

try
{ 
    o.parent();
}
catch(e)
{
    printStatus('Exception: ' + e);
}

TEST(1, expect, actual);
END();
