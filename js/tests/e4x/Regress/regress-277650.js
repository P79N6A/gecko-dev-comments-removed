






































var bug = 277650;
var summary = 'xml:lang attribute in XML literal';
var actual = '';
var expect = '';

START(summary);

printBugNumber (bug);
printStatus (summary);

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
