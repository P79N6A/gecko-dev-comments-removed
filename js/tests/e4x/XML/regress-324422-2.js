





































START("Do not crash creating XML object with long initialiser");

var bug = 324422;
var summary = 'Do not crash creating XML object with long initialiser';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

expectExitCode(0);
expectExitCode(3);

var str = '0123456789';

for (var icount = 0; icount < 24; icount++)
{
    str = str + str;
}

printStatus(str.length);

var x = new XML('<root>' + str + '</root>');

TEST(1, expect, actual);

END();
