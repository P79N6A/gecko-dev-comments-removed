




































gTestfile = 'regress-292863.js';

var summary = "Undeclaring namespace prefix should cause parse error";
var BUGNUMBER = 292863;
var actual = 'no error';
var expect = 'error';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    var godList = <gods:gods xmlns:gods="http://example.com/2005/05/04/gods">
        <god xmlns:god="">Kibo</god>
        </gods:gods>;
    printStatus(godList.toXMLString());
}
catch(e)
{
    actual = 'error';
}

TEST(1, expect, actual);

END();
