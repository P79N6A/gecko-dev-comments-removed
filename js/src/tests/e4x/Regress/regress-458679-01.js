






































var summary = 'GetXMLEntity should not assume FastAppendChar is infallible';
var BUGNUMBER = 458679;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expectExitCode(0);
expectExitCode(5);

try
{
    var x = "<";

    while (x.length < 12000000)
        x += x;

    <e4x>{x}</e4x>;
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
