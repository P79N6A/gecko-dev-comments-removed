








































gTestfile = 'regress-264369.js';

START("toXMLString() should escape '>'");
printBugNumber(264369);

var x = <a/>;
var chars = "<>&";
x.b = chars;

TEST(1, "<b>&lt;&gt;&amp;</b>", x.b.toXMLString());

END();
