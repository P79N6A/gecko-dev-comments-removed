









































gTestfile = '13.4.4.19.js';

START("13.4.4.19 - insertChildBefore()");

TEST(1, true, XML.prototype.hasOwnProperty("insertChildBefore"));
   
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

x.insertChildBefore(x.bravo[0], <delta>three</delta>);

TEST(2, correct, x);

x =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
</alpha>;

correct =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
    <delta>three</delta>
</alpha>;

x.insertChildBefore(null, <delta>three</delta>);

TEST(3, correct, x);


END();