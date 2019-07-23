





































gTestfile = 'regress-352223.js';

var BUGNUMBER = 352223;
var summary = 'Reject invalid spaces in tags';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'SyntaxError: invalid XML name';
try
{
    eval('< foo></foo>');
}
catch(ex)
{
    actual = ex + '';
}
TEST(1, expect, actual);

expect = 'SyntaxError: invalid XML tag syntax';
try
{
    eval('<foo></ foo>');
}
catch(ex)
{
    actual = ex + '';
}
TEST(2, expect, actual);

END();
