





































gTestfile = 'regress-327534.js';

var summary = "uneval on E4X gives Error: xml is not a function";
var BUGNUMBER = 327534;
var actual = '';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
START(summary);

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
