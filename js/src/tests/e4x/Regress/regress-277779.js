





































gTestfile = 'regress-277779.js';



var summary = 'call setNamespace on element with already existing default namespace';
var BUGNUMBER = 277779;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var xhtml2NS = new Namespace('http://www.w3.org/2002/06/xhtml2');
var xml = <html xmlns="http://www.w3.org/1999/xhtml" />;
xml.setNamespace(xhtml2NS);


expect = 'http://www.w3.org/2002/06/xhtml2';
actual = xml.namespace().toString();
TEST(1, expect, actual);

END();
