









































gTestfile = '13.5.4.5.js';

START("13.5.4.5 - XMLList children()");

TEST(1, true, XMLList.prototype.hasOwnProperty("children"));


x = new XMLList()
TEST(2, "xml", typeof(x.children()));
TEST_XML(3, "", x.children());


x += <alpha>one<bravo>two</bravo></alpha>;   
TEST(4, "xml", typeof(x.children()));

correct = <>one<bravo>two</bravo></>;
TEST(5, correct, x.children());


x += <charlie><bravo>three</bravo></charlie>;
TEST(6, "xml", typeof(x.children()));

correct = <>one<bravo>two</bravo><bravo>three</bravo></>;
TEST(7, correct, x.children());


x = new XMLList();
x += <alpha></alpha>;
x += <bravo></bravo>;
TEST(8, "xml", typeof(x.children()));
TEST_XML(9, "", x.children());



order =
<order>
    <customer>
        <name>John Smith</name>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
</order>;

correct= <price>1299.99</price> + <price>399.99</price>;

TEST(10, correct, order.children().price);

END();
