





































START("uneval on E4X gives Error: xml is not a function");

var bug = 327534;
var summary = 'uneval on E4X gives Error: xml is not a function';
var actual = '';
var expect = 'No Error';

printBugNumber (bug);
printStatus (summary);

try
{
    uneval(<x/>);
    actual = 'No Error';
}
catch(ex)
{
    printStatus(ex);
    actual = ex + '';
}
TEST(1, expect, actual);

END();
