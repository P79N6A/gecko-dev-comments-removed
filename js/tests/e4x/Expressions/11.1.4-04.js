




































gTestfile = '11.1.4-04.js';

var summary = "11.1.4 - XML Initializer - Comment hiding parsing/scanning";
var BUGNUMBER = 311157;
var actual;
var expect;

printBugNumber(BUGNUMBER);
START(summary);

var x = <hi> <!-- duh -->
    there </hi>;

actual = x.toString();
expect = '\n    there ';

TEST(1, expect, actual);

END();
