









































gTestfile = '13.4.4.20.js';

START("13.4.4.20 - XML length()");

TEST(1, true, XML.prototype.hasOwnProperty("length"));

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

TEST(2, 1, x.length());
TEST(3, 1, x.bravo.length());
TEST(4, 1, x.charlie.length());
TEST(5, 1, x.delta.length());

END();