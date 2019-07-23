




































gTestfile = '11.1.4-05.js';

var summary = "11.1.4 - XML Initializer - Comment hiding parsing/scanning";
var BUGNUMBER = 311157;
var actual;
var expect;

printBugNumber(BUGNUMBER);
START(summary);

XML.ignoreWhitespace = false;

var x = <bye> <![CDATA[ duh ]]>
    there </bye>;

actual = x.toString();
expect = '  duh \n    there ';
TEST(1, expect, actual);

END();
