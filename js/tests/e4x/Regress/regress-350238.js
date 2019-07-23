





































gTestfile = 'regress-350238.js';

var BUGNUMBER = 350238;
var summary = 'Do not assert <x/>.@*++';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

if (typeof document != 'undefined')
{
    document.location.href='javascript:<x/>.@*++;';
}
else
{
    <x/>.@*++;
}

TEST(1, expect, actual);

END();
