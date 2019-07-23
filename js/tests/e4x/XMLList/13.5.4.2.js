









































gTestfile = '13.5.4.2.js';

START("13.5.4.2 - XMLList attribute()");

TEST(1, true, XMLList.prototype.hasOwnProperty("attribute"));


emps = new XMLList();
TEST(2, "xml", typeof(emps.attribute("id")));
TEST_XML(3, "", emps.attribute("id"));


emps += <employee id="0"><name>Jim</name><age>25</age></employee>;

TEST(4, "xml", typeof(emps.attribute("id")));
TEST_XML(5, 0, emps.attribute("id"));


emps += <employee id="1"><name>Joe</name><age>20</age></employee>;
TEST(6, "xml", typeof(emps.attribute("id")));

correct = new XMLList();
correct += new XML("0");
correct += new XML("1");
TEST(7, correct, emps.attribute("id"));


TEST(8, "xml", typeof(emps.attribute("foobar")));


try {
  emps.attribute(null);
  SHOULD_THROW(9);
} catch (ex) {
  TEST(9, "TypeError", ex.name);
}

try {
  emps.attribute(undefined);
  SHOULD_THROW(10);
} catch (ex) {
  TEST(10, "TypeError", ex.name);
}

END();