




































gTestfile = 'regress-313080.js';

var summary = "Regression - Do not crash calling __proto__, __parent__";
var BUGNUMBER = 313080;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    <element/>.__proto__();
    <element/>.__parent__();
    <element/>.function::__proto__();
}
catch(e)
{
    printStatus(e + '');
}
TEST(1, expect, actual);

END();
