





































gTestfile = 'regress-356238-01.js';

var BUGNUMBER = 356238;
var summary = 'bug 356238';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

try
{
	<xml/>.replace(1);
}
catch(ex)
{
}
TEST(1, expect, actual);
END();
