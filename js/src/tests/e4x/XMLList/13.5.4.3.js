









































gTestfile = '13.5.4.3.js';

START("13.5.4.3 - XMLList attributes()");

TEST(1, true, XMLList.prototype.hasOwnProperty("attributes"));


x = new XMLList()
TEST(2, "xml", typeof(x.attributes()));
TEST_XML(3, "", x.attributes());


x += <alpha attr1="value1" attr2="value2">one</alpha>;

TEST(4, "xml", typeof(x.attributes()));
correct = new XMLList();
correct += new XML("value1");
correct += new XML("value2");
TEST(5, correct, x.attributes());


x += <bravo attr3="value3" attr4="value4">two</bravo>;

TEST(6, "xml", typeof(x.attributes()));
correct = new XMLList();
correct += new XML("value1");
correct += new XML("value2");
correct += new XML("value3");
correct += new XML("value4");
TEST(7, correct, x.attributes());

END();