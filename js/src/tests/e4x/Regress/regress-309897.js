




































gTestfile = 'regress-309897.js';

var summary = "Regression - appending elements crashes mozilla";
var BUGNUMBER = 309897;
var actual = "No Crash";
var expect = "No Crash";

printBugNumber(BUGNUMBER);
START(summary);

function crash()
{
    try
    {
        var top = <top/>;
        for (var i = 0; i < 1000; i++)
        {
            top.stuff += <stuff>stuff</stuff>;
        }
    }
    catch(e)
    {
        printStatus("Exception: " + e);
    }
}

crash();

TEST(1, expect, actual);

END();
