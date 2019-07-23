




































gTestfile = '11.1.4-02.js';

var summary = "11.1.4 - XML Initializer should accept single CDATA Section";
var BUGNUMBER = 257679;
var actual = '';
var expect = 'text';

printBugNumber(BUGNUMBER);
START(summary);

var cdataText = <![CDATA[Kibology for all.<br>All for Kibology.]]>;
if (cdataText) {
    actual = cdataText.nodeKind();
}
else {
    actual = 'undefined';
}

TEST(1, expect, actual);

END();
