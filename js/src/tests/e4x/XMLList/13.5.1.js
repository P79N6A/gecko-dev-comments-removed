









































gTestfile = '13.5.1.js';

START("13.5.1 - XMLList Constructor as Function");

x = XMLList();

TEST(1, "xml", typeof(x));  
TEST(2, true, x instanceof XMLList);


x =
<>
    <alpha>one</alpha>
    <bravo>two</bravo>
</>;

y = XMLList(x);
TEST(3, x === y, true);

x += <charlie>three</charlie>;
TEST(4, x === y, false);


x = XMLList(<alpha>one</alpha>);
TEST_XML(5, "<alpha>one</alpha>", x);


x = XMLList(<><alpha>one</alpha><bravo>two</bravo></>);
correct = new XMLList();
correct += <alpha>one</alpha>;
correct += <bravo>two</bravo>;
TEST_XML(6, correct.toString(), x);


x = XMLList(<><alpha>one</alpha><bravo>two</bravo></>);
correct = new XMLList();
correct += <alpha>one</alpha>;
correct += <bravo>two</bravo>;
TEST_XML(7, correct.toString(), x);


x = XMLList("foobar");
TEST_XML(8, "foobar", x);

x = XMLList(7);
TEST_XML(9, "7", x);


x = XMLList(null);
TEST_XML(10, "", x);

x = XMLList(undefined);
TEST_XML(11, "", x);

END();