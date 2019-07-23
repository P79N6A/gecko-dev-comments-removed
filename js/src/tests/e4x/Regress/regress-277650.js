





































gTestfile = 'regress-277650.js';



var summary = 'xml:lang attribute in XML literal';
var BUGNUMBER = 277650;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'no error';
try
{
    var xml = <root><text xml:lang="en">ECMAScript for XML</text></root>;
    actual = 'no error';
}
catch(e)
{
    actual = 'error';
}

TEST(1, expect, actual);

END();
