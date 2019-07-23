





































var bug = 340024;
var summary = '11.1.4 - XML Initializer';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = '<tag b="c" d="e"/>';
try
{
    actual = (<tag {0?"a":"b"}="c" d="e"/>.toXMLString());
}
catch(E)
{
    actual = E + '';
}

TEST(1, expect, actual);

END();
