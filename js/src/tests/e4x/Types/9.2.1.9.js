









































gTestfile = '9.2.1.9.js';

START("9.2.1.9 XMLList [[Equals]]");


TEST(1, true, (new XMLList() == undefined) && (undefined == new XMLList()));


x = <alpha>one</alpha> + <bravo>two</bravo>;
y = <alpha>one</alpha> + <bravo>two</bravo>;
TEST(2, true, (x == y) && (y == x));
y = <alpha>one</alpha> + <bravo>two</bravo> + <charlie>three</charlie>;
TEST(3, false, (x == y) || (y == x));
y = <alpha>one</alpha> + <bravo>not</bravo>;
TEST(4, false, (x == y) || (y == x));


x = new XMLList(<alpha>one</alpha>);
y = <alpha>one</alpha>;
TEST(5, true, (x == y) && (y == x));
y = "one";
TEST(6, true, (x == y) && (y == x));


x = <alpha>one</alpha> + <bravo>two</bravo>;
y = <alpha>one</alpha>;
TEST(7, false, (x == y) && (y == x));

y = "<alpha>one</alpha>";
TEST(8, false, (x == y) || (y == x));


y = null;
TEST(9, false, (x == y) || (y == x));

y = new Object();
TEST(10, false, (x == y) || (y == x));

END();