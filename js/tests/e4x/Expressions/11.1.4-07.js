




































gTestfile = '11.1.4-07.js';

var summary = "11.1.4 - XML Initializer - <p:{b}b>x</p:bb>";
var BUGNUMBER = 321549;
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
START(summary);

var b = 'b';

try
{
    actual = (<a xmlns:p='http://a.uri/'><p:b{b}>x</p:bb></a>).
        toString();
}
catch(e)
{
    actual = e + '';
}

expect = (<a xmlns:p='http://a.uri/'><p:bb>x</p:bb></a>).toString();

TEST(1, expect, actual);

try
{
    actual = (<a xmlns:p='http://a.uri/'><p:{b}b>x</p:bb></a>).
        toString();
}
catch(e)
{
    actual = e + '';
}

expect = (<a xmlns:p='http://a.uri/'><p:bb>x</p:bb></a>).toString();

TEST(2, expect, actual);

END();
