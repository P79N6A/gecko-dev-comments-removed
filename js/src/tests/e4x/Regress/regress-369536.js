






































gTestfile = 'regress-369536.js';

START("Assignment to XML property removes attributes");
printBugNumber(369536);

var x = <foo><bar id="1">bazOne</bar><bar id="2">bazTwo</bar></foo>;
TEST_XML(1, "<bar id=\"2\">bazTwo</bar>", x.bar[1]);
x.bar[1] = "bazTwoChanged";
TEST_XML(2, "<bar id=\"2\">bazTwoChanged</bar>", x.bar[1]);
