




































START("11.1.4 - XML Initializer");

var bug = 257679;
var summary = 'XML Initializer should accept single CDATA Section';
var actual = '';
var expect = 'text';

printBugNumber (bug);
printStatus (summary);

var cdataText = <![CDATA[Kibology for all.<br>All for Kibology.]]>;
if (cdataText) {
    actual = cdataText.nodeKind();
}
else {
    actual = 'undefined';
}

TEST(1, expect, actual);

END();
