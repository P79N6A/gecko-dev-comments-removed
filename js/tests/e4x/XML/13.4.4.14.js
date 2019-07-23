









































gTestfile = '13.4.4.14.js';

START("13.4.4.14 - XML hasOwnProperty()");

TEST(1, true, XML.prototype.hasOwnProperty("hasOwnProperty"));
   
x =
<alpha attr1="value1">
    <bravo>one</bravo>
    <charlie>
        two
        three
        <echo>four</echo>
    </charlie>
    <delta />
</alpha>;


TEST(2, true, x.hasOwnProperty("bravo"));
TEST(3, true, x.hasOwnProperty("@attr1"));
TEST(4, false, x.hasOwnProperty("foobar"));


TEST(5, true, XML.prototype.hasOwnProperty("toString"));
TEST(6, false, XML.prototype.hasOwnProperty("foobar"));

END();