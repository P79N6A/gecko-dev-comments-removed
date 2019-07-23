




































gTestfile = 'regress-311580.js';

var summary = "Regression - properly root stack in toXMLString";
var BUGNUMBER = 311580;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);


var xmlOl = new XML('<ol><li>Item 1<\/li><li>Item 2<\/li><\/ol>');
var list =  xmlOl.li;

for(i = 0; i < 30000; i++)
{
    list[i+2] = "Item " + (i+3); 
}

var s = xmlOl.toXMLString();

TEST(1, expect, actual);

END();
