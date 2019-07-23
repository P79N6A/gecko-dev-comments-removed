









































gTestfile = '13.4.4.12.js';

START("13.4.4.12 - XML descendants");

TEST(1, true, XML.prototype.hasOwnProperty("descendants"));

x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        two
        <bravo>three</bravo>
    </charlie>
</alpha>;

TEST(2, <bravo>three</bravo>, x.charlie.descendants("bravo"));
TEST(3, <><bravo>one</bravo><bravo>three</bravo></>, x.descendants("bravo"));


correct = <><bravo>one</bravo>one<charlie>two<bravo>three</bravo></charlie>two<bravo>three</bravo>three</>;

XML.prettyPrinting = false;
TEST(4, correct, x.descendants("*"));
TEST(5, correct, x.descendants());
XML.prettyPrinting = true;

END();
