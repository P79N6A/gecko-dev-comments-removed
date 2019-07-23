





































gTestfile = 'regress-458679-01.js';

var summary = 'GetXMLEntity should not assume FastAppendChar is infallible';
var BUGNUMBER = 458679;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

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
