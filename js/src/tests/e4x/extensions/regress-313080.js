





































var summary = "Regression - Do not crash calling __proto__";
var BUGNUMBER = 313080;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    <element/>.__proto__();
    <element/>.function::__proto__();
}
catch(e)
{
    printStatus(e + '');
}
TEST(1, expect, actual);

END();
