









































gTestfile = '10.4.1.js';

START("10.4.1 - toXMLList Applied to String type");

var x, y, correct;

x =
<>
    <alpha>one</alpha>
    <bravo>two</bravo>
</>;

TEST(1, "xml", typeof(x));
TEST(2, "<alpha>one</alpha>\n<bravo>two</bravo>", x.toXMLString());



y = new XMLList(x);

x += <charlie>three</charlie>;

TEST(3, "<alpha>one</alpha>\n<bravo>two</bravo>", y.toXMLString());
  

x = new XMLList(<alpha>one</alpha>);
TEST(4, "<alpha>one</alpha>", x.toXMLString());


x = new XMLList(<><alpha>one</alpha><bravo>two</bravo></>);
TEST(5, "<alpha>one</alpha>\n<bravo>two</bravo>", x.toXMLString());


x = new XMLList("<alpha>one</alpha><bravo>two</bravo>");
TEST(6, "<alpha>one</alpha>\n<bravo>two</bravo>", x.toXMLString());





John = "<employee><name>John</name><age>25</age></employee>";
Sue = "<employee><name>Sue</name><age>32</age></employee>";

correct =
<>
    <employee><name>John</name><age>25</age></employee>
    <employee><name>Sue</name><age>32</age></employee>
</>;

var1 = new XMLList(John + Sue);

TEST(8, correct, var1);

END();