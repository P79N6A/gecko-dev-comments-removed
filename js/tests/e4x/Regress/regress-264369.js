








































START("toXMLString() should escape '>'");
BUG(264369);

var x = <a/>;
var chars = "<>&";
x.b = chars;

TEST(1, "<b>&lt;&gt;&amp;</b>", x.b.toXMLString());

END();
