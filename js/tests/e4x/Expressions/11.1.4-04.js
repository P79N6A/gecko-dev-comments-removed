




































START("11.1.4 - XML Initializer - Comment hiding parsing/scanning");

var bug = 311157;
var summary = 'Comment-hiding compromise left E4X parsing/scanning inconsistent';
var actual;
var expect;

printBugNumber (bug);
printStatus (summary);


var x = <hi> <!-- duh -->
    there </hi>;

actual = x.toString();
expect = '\n    there ';

TEST(1, expect, actual);

END();
