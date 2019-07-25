






START("13.4.4.18 - XML insertChildAfter()");

TEST(1, true, XML.prototype.hasOwnProperty("insertChildAfter"));

x =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
</alpha>;

correct =
<alpha>
    <bravo>one</bravo>
    <delta>three</delta>
    <charlie>two</charlie>
</alpha>;

x.insertChildAfter(x.bravo[0], <delta>three</delta>);

TEST(2, correct, x);

x =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
</alpha>;

correct =
<alpha>
    <delta>three</delta>
    <bravo>one</bravo>
    <charlie>two</charlie>
</alpha>;

x.insertChildAfter(null, <delta>three</delta>);

TEST(3, correct, x);


END();
