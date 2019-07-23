




































START("Regression - appending elements crashes mozilla");

var bug = 309897;
var summary = "appending elements crashes mozilla";
var actual = "No Crash";
var expect = "No Crash";

printBugNumber (bug);
printStatus (summary);

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
