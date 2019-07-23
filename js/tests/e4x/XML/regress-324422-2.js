





































gTestfile = 'regress-324422-2.js';

var summary = "Do not crash creating XML object with long initialiser";
var BUGNUMBER = 324422;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);
printStatus ("Expect out of memory or script stack space quota is exhausted error");

expectExitCode(0);
expectExitCode(3);

try
{
    var str = '0123456789';

    for (var icount = 0; icount < 24; icount++)
    {
        str = str + str;
    }

    printStatus(str.length);

    var x = new XML('<root>' + str + '</root>');
}
catch(ex)
{
    print(ex + '');
}
TEST(1, expect, actual);

END();
