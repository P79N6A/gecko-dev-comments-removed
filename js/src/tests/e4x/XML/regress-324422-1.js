






































var summary = "Do not crash creating XML object with long initialiser";

var BUGNUMBER = 324422;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

expectExitCode(0);
expectExitCode(5);

if (typeof document == 'undefined')
{
    printStatus ("Expect possible out of memory error");
    expectExitCode(0);
    expectExitCode(5);
}
var str = '<fu>x</fu>';

for (var icount = 0; icount < 20; icount++)
{
    str = str + str;
}

printStatus(str.length);

var x = new XML('<root>' + str + '</root>');

TEST(1, expect, actual);

END();
