









































gTestfile = 'regress-496113.js';

var summary = 'simple filter';
var BUGNUMBER = 496113;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var people = <people>
 <person>
  <name>Joe</name>
 </person>
</people>;

expect = <person><name>Joe</name></person>.toXMLString();

print(actual = people.person.(name == "Joe").toXMLString());

TEST(1, expect, actual);

END();
