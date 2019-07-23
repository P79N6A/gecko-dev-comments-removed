






































var bug = 277779;
var summary = 'call setNamespace on element with already existing default namespace';
var actual = '';
var expect = '';

START(summary);

printBugNumber (bug);
printStatus (summary);

var xhtml2NS = new Namespace('http://www.w3.org/2002/06/xhtml2');
var xml = <html xmlns="http://www.w3.org/1999/xhtml" />;
xml.setNamespace(xhtml2NS);


expect = 'http://www.w3.org/2002/06/xhtml2';
actual = xml.namespace().toString();
TEST(1, expect, actual);

END();
