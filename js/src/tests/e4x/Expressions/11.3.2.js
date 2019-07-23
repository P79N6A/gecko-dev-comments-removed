









































gTestfile = '11.3.2.js';

START("11.3.2 - Typeof Operator");

x = new XML();
TEST(1, "xml", typeof(x));
x = new XMLList();
TEST(2, "xml", typeof(x));

END();