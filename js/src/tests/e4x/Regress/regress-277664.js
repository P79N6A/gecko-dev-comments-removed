





































gTestfile = 'regress-277664.js';



var summary = 'duplicate attribute names';
var BUGNUMBER = 277664;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'error';
try
{
    var god = <god name="Kibo" name="Xibo" />;
    actual = 'no error';
}
catch(e)
{
    actual = 'error';
}

TEST(1, expect, actual);

END();
