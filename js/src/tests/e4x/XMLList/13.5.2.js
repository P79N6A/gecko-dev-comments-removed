









































gTestfile = '13.5.2.js';

START("13.5.2 - XMLList Constructor");

x = new XMLList();
TEST(1, "xml", typeof(x));  
TEST(2, true, x instanceof XMLList);



x =
<>
    <alpha>one</alpha>
    <bravo>two</bravo>
</>;

y = new XMLList(x);

x += <charlie>three</charlie>;

TEST(3, "<alpha>one</alpha>\n<bravo>two</bravo>", y.toString());
  

x = new XMLList(<alpha>one</alpha>);
TEST_XML(4, "<alpha>one</alpha>", x);


x = new XMLList(<><alpha>one</alpha><bravo>two</bravo></>);
TEST(5, "<alpha>one</alpha>\n<bravo>two</bravo>", x.toString());


x = new XMLList(<><alpha>one</alpha><bravo>two</bravo></>);
TEST(6, "<alpha>one</alpha>\n<bravo>two</bravo>", x.toString());


x = new XMLList("foobar");
TEST_XML(7, "foobar", x);

x = XMLList(7);
TEST_XML(8, 7, x);


x = new XMLList(null);
TEST_XML(9, "", x);

x = new XMLList(undefined);
TEST_XML(10, "", x);

END();