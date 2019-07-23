




































START("11.1.4 - XML Initializer");

var bug = 257679;
var summary = 'XML Initializer should accept single processing instruction';
var actual = '';
var expect = 'processing-instruction';

printBugNumber (bug);
printStatus (summary);

XML.ignoreProcessingInstructions = false;
print("XML.ignoreProcessingInstructions: " + XML.ignoreProcessingInstructions);
var pi = <?process Kibology="on"?>;
if (pi) {
    actual = pi.nodeKind();
}
else {
    actual = 'undefined';
}

TEST(1, expect, actual);

END();
