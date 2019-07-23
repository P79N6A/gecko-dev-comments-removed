




































START("Regression - Do not crash calling __proto__, __parent__");

var bug = 313080;
var summary = 'Do not crash calling __proto__, __parent__';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

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
