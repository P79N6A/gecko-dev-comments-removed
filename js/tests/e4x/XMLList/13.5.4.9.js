










































gTestfile = '13.5.4.9.js';

START("13.5.4.9 - XMLList descendants()");

TEST(1, true, XMLList.prototype.hasOwnProperty("descendants"));



var gods = <gods>
  <god>
    <name>Kibo</name>
  </god>
  <god>
    <name>Xibo</name>
  </god>
</gods>;

var godList = gods.god;

var expect;
var actual;
var node;
var descendants = godList.descendants();

expect = 4;
actual = descendants.length();
TEST(2, expect, actual)

expect = 'nodeKind(): element, name(): name;\n' +
         'nodeKind(): text, name(): null;\n' +
         'nodeKind(): element, name(): name;\n' +
         'nodeKind(): text, name(): null;\n';

actual = '';

for each (var xml in descendants) {
  actual += 'nodeKind(): ' + xml.nodeKind() + ', name(): ' + xml.name() + ';\n';
}
TEST(4, expect, actual);

END();
