









































gTestfile = '10.4.js';

START("10.4 - toXMLList");

var x;


try {
    x = null;
    x.toString();
    SHOULD_THROW(1);
} catch (ex) {
    TEST(1, "TypeError", ex.name);
}


x = new Number(123);
TEST(2, "123", new XMLList(x).toXMLString());


x = new String("<alpha><bravo>one</bravo></alpha>");
TEST(3, <alpha><bravo>one</bravo></alpha>, new XMLList(x));

x = new String("<alpha>one</alpha><charlie>two</charlie>");
TEST(4, "<alpha>one</alpha>\n<charlie>two</charlie>",
  new XMLList(x).toXMLString());


x = new XML(<alpha><bravo>one</bravo></alpha>);
TEST(5, <alpha><bravo>one</bravo></alpha>, new XMLList(x));

x = new XML(<root><alpha><bravo>one</bravo></alpha><charlie>two</charlie></root>);
TEST(6, <alpha><bravo>one</bravo></alpha>, new XMLList(x.alpha));


x = new XMLList(<alpha><bravo>one</bravo></alpha>);
TEST(7, <alpha><bravo>one</bravo></alpha>, new XMLList(x));

x = new XMLList(<><alpha>one</alpha><bravo>two</bravo></>);
TEST(8, "<alpha>one</alpha>\n<bravo>two</bravo>",
  new XMLList(x).toXMLString());
   
END();