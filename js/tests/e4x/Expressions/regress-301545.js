




































START("11.1.1 - Attribute Identifiers");

var bug = 301545;
var summary = 'Do not crash when attribute-op name collides with local var';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

function TOCParser(aElement) {
  var href = aElement.@href;
}

TEST(summary, expect, actual);

END();
