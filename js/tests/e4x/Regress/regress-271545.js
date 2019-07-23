








































gTestfile = 'regress-271545.js';

START('XML("") should create empty text node');
printBugNumber(271545);




var x;

x = new XML();
x.a = "foo";
TEST_XML(1, "", x);

x = new XML("");
x.a = "foo";
TEST_XML(2, "", x);

x = new XML(null);
x.a = "foo";
TEST_XML(3, "", x);

x = new XML(undefined);
x.a = "foo";
TEST_XML(4, "", x);

var textNodeContent = "some arbitrary text without XML markup";

x = new XML(textNodeContent);
x.a = "foo";
TEST_XML(5, textNodeContent, x);

END();
