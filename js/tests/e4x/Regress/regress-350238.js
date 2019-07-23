





































var bug = 350238;
var summary = 'Do not assert <x/>.@*++';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

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
