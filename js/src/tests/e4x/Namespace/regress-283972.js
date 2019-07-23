





































gTestfile = 'regress-283972.js';

var summary = 'throw error when two attributes with the same local name and ' +
    'the same namespace';
var BUGNUMBER = 283972;
var actual = 'no error';
var expect = 'error';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    var xml = <god xmlns:pf1="http://example.com/2005/02/pf1"
        xmlns:pf2="http://example.com/2005/02/pf1"
        pf1:name="Kibo"
        pf2:name="Xibo" />;
    printStatus(xml.toXMLString());
}
catch(e)
{
    actual = 'error';
}

TEST(1, expect, actual);

END();
