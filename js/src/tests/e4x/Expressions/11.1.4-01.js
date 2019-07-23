




































gTestfile = '11.1.4-01.js';

var summary = '11.1.4 - XML Initializer should accept single processing ' +
    'instruction';
var BUGNUMBER = 257679;
var actual = '';
var expect = 'processing-instruction';

printBugNumber(BUGNUMBER);
START(summary);

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
