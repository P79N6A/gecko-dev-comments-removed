









































gTestfile = '13.4.4.40.js';

START("13.4.4.40 - valueOf");

TEST(1, true, XML.prototype.hasOwnProperty("valueOf"));

x =
<alpha>
    <bravo>one</bravo>
</alpha>;

TEST(2, x, x.valueOf());



x =
<alpha>
    <bravo>one</bravo>
</alpha>;

y = x.valueOf();

x.bravo = "two";

TEST(3, x, y);

END();