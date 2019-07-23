




































START("11.1.4 - XML Initializer - Comment hiding parsing/scanning");

var bug = 311157;
var summary = 'Comment-hiding compromise left E4X parsing/scanning inconsistent';
var actual;
var expect;

printBugNumber (bug);
printStatus (summary);

XML.ignoreWhitespace = false;

var x = <bye> <![CDATA[ duh ]]>
    there </bye>;

actual = x.toString();
expect = '  duh \n    there ';
TEST(1, expect, actual);

END();
