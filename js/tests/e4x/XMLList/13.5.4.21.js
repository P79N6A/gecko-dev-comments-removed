









































gTestfile = '13.5.4.21.js';

START("13.5.4.21 - XMLList toXMLString()");

TEST(1, true, XMLList.prototype.hasOwnProperty("toXMLString"));
   
x = <><alpha>one</alpha></>;

TEST(2, "<alpha>one</alpha>", x.toXMLString());

x = <><alpha>one</alpha><bravo>two</bravo></>;

TEST(3, "<alpha>one</alpha>\n<bravo>two</bravo>", x.toXMLString());

END();