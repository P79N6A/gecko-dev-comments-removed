





































gTestfile = 'regress-301553.js';

var summary = "E4X - Should not repress exceptions";
var BUGNUMBER = 301553;
var actual = 'No exception';
var expect = 'exception';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    var x = <xml/>;
    Object.toString.call(x);
}
catch(e)
{
    actual = 'exception';
}

TEST(1, expect, actual);
END();
