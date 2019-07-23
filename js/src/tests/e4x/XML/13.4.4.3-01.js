




































gTestfile = '13.4.4.3-01.js';

var summary = "13.4.4.3 - XML.appendChild should copy child";

var BUGNUMBER = 312692;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var ul = <ul/>;
var li = <li/>;

li.setChildren("First");
ul.appendChild(li);

li.setChildren("Second");
ul.appendChild(li);

XML.ignoreWhitespace = true;
XML.prettyPrinting = true;

expect = (
  <ul>
    <li>Second</li>
    <li>Second</li>
  </ul>
    ).toXMLString().replace(/</g, '&lt;');

actual = ul.toXMLString().replace(/</g, '&lt;');

TEST(1, expect, actual);

END();
