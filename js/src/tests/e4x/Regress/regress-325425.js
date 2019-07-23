





































gTestfile = 'regress-325425.js';

var BUGNUMBER = 325425;
var summary = 'jsxml.c: Bad assumptions about js_ConstructObject';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    QName = function() { };
    <xml/>.elements("");
}
catch(ex)
{
    printStatus(ex + '');
}
TEST(1, expect, actual);

END();
