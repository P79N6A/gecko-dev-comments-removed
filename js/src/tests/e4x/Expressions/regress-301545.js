




































gTestfile = 'regress-301545.js';

var summary = "11.1.1 - Attribute Identifiers Do not crash when " +
    "attribute-op name collides with local var";
var BUGNUMBER = 301545;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

function TOCParser(aElement) {
  var href = aElement.@href;
}

TEST(summary, expect, actual);

END();
