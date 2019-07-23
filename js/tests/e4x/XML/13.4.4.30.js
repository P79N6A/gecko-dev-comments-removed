









































gTestfile = '13.4.4.30.js';

START("13.4.4.30 - propertyIsEnumerable()");

TEST(1, true, XML.prototype.hasOwnProperty("propertyIsEnumerable"));


x =
<alpha>
    <bravo>one</bravo>
</alpha>;

TEST(2, true, x.propertyIsEnumerable("0"));
TEST(3, true, x.propertyIsEnumerable(0));
TEST(5, false, x.propertyIsEnumerable("bravo"));
TEST(6, false, x.propertyIsEnumerable());
TEST(7, false, x.propertyIsEnumerable(undefined));
TEST(8, false, x.propertyIsEnumerable(null));

END();