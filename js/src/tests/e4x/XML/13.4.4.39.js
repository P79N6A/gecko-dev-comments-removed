










































START("13.4.4.39 - XML toXMLString");

TEST(1, true, XML.prototype.hasOwnProperty("toXMLString"));

XML.prettyPrinting = false;

x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        <bravo>two</bravo>
    </charlie>
</alpha>;

TEST(2, "<bravo>one</bravo>", x.bravo.toXMLString());
TEST(3, "<bravo>one</bravo><bravo>two</bravo>", x..bravo.toXMLString());

x =
<alpha>
    <bravo>one</bravo>
    <charlie/>
</alpha>;

TEST(4, "<charlie/>", x.charlie.toXMLString());

x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        <bravo>two</bravo>
    </charlie>
</alpha>;

TEST(5, "<charlie><bravo>two</bravo></charlie>", x.charlie.toXMLString());

x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        two
        <bravo/>
    </charlie>
</alpha>;

TEST(6, "<charlie>two<bravo/></charlie>", x.charlie.toXMLString());

x =
<alpha>
    <bravo></bravo>
    <bravo/>
</alpha>;

TEST(7, "<bravo/><bravo/>", x.bravo.toXMLString());

x =
<alpha>
    <bravo>one<charlie/></bravo>
    <bravo>two<charlie/></bravo>
</alpha>;

TEST(8, "<bravo>one<charlie/></bravo><bravo>two<charlie/></bravo>", x.bravo.toXMLString());
  
XML.prettyPrinting = true;

x =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
</alpha>;

copy = x.bravo.copy();

TEST(9, "<bravo>one</bravo>", copy.toXMLString());

x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        <bravo>two</bravo>
    </charlie>
</alpha>;

TEST(10, "String contains value one from bravo", "String contains value " + x.bravo + " from bravo");

END();
