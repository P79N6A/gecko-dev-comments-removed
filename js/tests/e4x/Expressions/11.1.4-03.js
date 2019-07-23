




































START("11.1.4 - XML Initializer");

var bug = 257679;
var summary = 'XML Initializer should accept single comment';
var actual = '';
var expect = 'comment';

printBugNumber (bug);
printStatus (summary);

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
