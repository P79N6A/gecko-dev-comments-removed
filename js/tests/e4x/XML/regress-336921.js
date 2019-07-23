





































START("13.4.4.3 - XML.prototype.appendChild");

var bug = 336921;
var summary = 'XML.prototype.appendChild creates undesired <br/>';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);



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
