





































gTestfile = 'regress-336921.js';

var summary = '13.4.4.3 - XML.prototype.appendChild creates undesired <br/>';
var BUGNUMBER = 336921;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);



var array = ["a","b","c"];
var myDiv = <div/>;
for each (a in array) {
    myDiv.appendChild(a);
    myDiv.appendChild(<br/>);
}

actual = myDiv.toXMLString();
expect = '<div>\n  a\n  <br/>\n  b\n  <br/>\n  c\n  <br/>\n</div>';

TEST(1, expect, actual);

END();
