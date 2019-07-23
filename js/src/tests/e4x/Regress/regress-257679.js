







































gTestfile = 'regress-257679.js';

START("Standalone <![CDATA[ .... ]]> should be allowed");
printBugNumber(257679);

var x = <![CDATA[ < some & > arbitrary text ]]>;

var expected = new XML("<![CDATA[ < some & > arbitrary text ]]>");

TEST(1, expected, x);

END();
