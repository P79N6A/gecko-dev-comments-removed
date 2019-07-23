









































gTestfile = '13.5.4.11.js';

START("13.5.4.11 - XMLList hasOwnProperty()");

TEST(1, true, XMLList.prototype.hasOwnProperty("hasOwnProperty"));
 

x =
<>
<alpha attr1="value1">
    <bravo>one</bravo>
    <charlie>
        two
        three
        <echo>four</echo>
    </charlie>
    <delta />
</alpha>
<delta>
    <echo>five</echo>
</delta>
</>;


TEST(2, true, x.hasOwnProperty("bravo"));
TEST(3, true, x.hasOwnProperty("@attr1"));
TEST(4, false, x.hasOwnProperty("foobar"));
TEST(5, true, x.hasOwnProperty("echo"));


TEST(5, true, XMLList.prototype.hasOwnProperty("toString"));
TEST(6, false, XMLList.prototype.hasOwnProperty("foobar"));
 
END();