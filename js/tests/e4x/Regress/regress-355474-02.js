





































var bug = 355474;
var summary = 'Iterating over XML with WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = '<a>text</a>';
for each (var i in <><a>text</a></>) 
{
    printStatus(i.toXMLString());
    actual = i.toXMLString();
}

TEST(1, expect, actual);

END();
