




































START("E4X QuoteString should deal with empty string");

var bug = 302531;
var summary = 'E4X QuoteString should deal with empty string';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

function f(e) {
  return <e {e}="" />;
}

XML.ignoreWhitespace = true;
XML.prettyPrinting = true;

expect = (
    <e foo="" />
    ).toXMLString().replace(/</g, '&lt;');

actual = f('foo').toXMLString().replace(/</g, '&lt;');

TEST(1, expect, actual);

END();
