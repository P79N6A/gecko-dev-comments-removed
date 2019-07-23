






































var bug = 277664;
var summary = 'duplicate attribute names';
var actual = '';
var expect = '';

START(summary);

printBugNumber (bug);
printStatus (summary);

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
