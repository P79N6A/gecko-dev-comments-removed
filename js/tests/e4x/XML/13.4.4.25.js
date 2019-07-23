









































gTestfile = '13.4.4.25.js';

START("13.4.4.25 - XML nodeKind()");

TEST(1, true, XML.prototype.hasOwnProperty("nodeKind"));

x =
<alpha attr1="value1">
    <bravo>one</bravo>
</alpha>;

TEST(2, "element", x.bravo.nodeKind());
TEST(3, "attribute", x.@attr1.nodeKind());


x = new XML();
TEST(4, "text", x.nodeKind());
TEST(5, "text", XML.prototype.nodeKind());

END();