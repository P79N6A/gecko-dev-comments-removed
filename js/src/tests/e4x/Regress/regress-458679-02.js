






































var summary = 'GetXMLEntity should not assume FastAppendChar is infallible';
var BUGNUMBER = 458679;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expectExitCode(0);
expectExitCode(5);

function stringOfLength(n)
{
    if (n == 0) {
        return "";
    } else if (n == 1) {
        return "<";
    } else {
        var r = n % 2;
        var d = (n - r) / 2;
        var y = stringOfLength(d);
        return y + y + stringOfLength(r);
    }    
}

try
{

    void stringOfLength(4435455);
    x = stringOfLength(14435455);
    <xxx>{x}</xxx>;
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
