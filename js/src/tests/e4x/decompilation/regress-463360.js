








var summary = 'Uneval+eval of XML containing string with {';
var BUGNUMBER = 463360;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

actual = eval(uneval(XML("<a>{</a>")));
expect = XML("<a>{</a>");

compareSource(expect, actual, inSection(1) + summary);

END();
