




































gTestfile = '11.1.4-03.js';

var summary = '11.1.4 - XML Initializer should accept single comment';
var BUGNUMBER = 257679;
var actual = '';
var expect = 'comment';

printBugNumber(BUGNUMBER);
START(summary);

XML.ignoreComments = false;
print("XML.ignoreComments: " + XML.ignoreComments);
var comment = <!-- comment -->;
if (comment) {
    actual = comment.nodeKind();
}
else {
    actual = 'undefined';
}

TEST(1, expect, actual);

END();
