









































gTestfile = '9.1.1.9.js';

START("9.1.1.9 - XML [[Equals]]");

x = <alpha>one</alpha>;
y = <alpha>one</alpha>;
TEST(1, true, (x == y) && (y == x));


y = "<alpha>one</alpha>";
TEST(2, false, (x == y) || (y == x));

y = undefined
TEST(3, false, (x == y) || (y == x));

y = null
TEST(4, false, (x == y) || (y == x));

y = new Object();
TEST(5, false, (x == y) || (y == x));


x = <alpha attr1="value1">one<bravo attr2="value2">two</bravo></alpha>;
y = <alpha attr1="value1">one<bravo attr2="value2">two</bravo></alpha>;
TEST(6, true, (x == y) && (y == x));

y = <alpha attr1="new value">one<bravo attr2="value2">two</bravo></alpha>;
TEST(7, false, (x == y) || (y == x));



x = <alpha attr1="value1" attr2="value2">one<bravo attr3="value3" attr4="value4">two</bravo></alpha>;
y = <alpha attr2="value2" attr1="value1">one<bravo attr4="value4" attr3="value3">two</bravo></alpha>;
TEST(8, true, (x == y) && (y == x));



x = <alpha> <bravo>one</bravo> </alpha>;
y = <alpha><bravo>one</bravo></alpha>;
TEST(9, true, (x == y) && (y == x));


x = <alpha><bravo> one </bravo></alpha>;
y = <alpha><bravo>one</bravo></alpha>;
TEST(10, false, (x == y) || (y == x));


XML.ignoreComments = false;
x = <alpha><!-- comment1 --><bravo><!-- comment2 -->one</bravo></alpha>;
y = <alpha><!-- comment2 --><bravo><!-- comment1 -->one</bravo></alpha>;
TEST(11, false, (x == y) || (y == x));

one = x.*[0];
two = y.*[0];
TEST(12, false, (one == two) || (two == one));

one = x.*[0];
two = y.bravo.*[0];
TEST(13, true, (one == two) && (two == one));



XML.ignoreProcessingInstructions = false;
x = <alpha><?one foo="bar" ?><bravo><?two bar="foo" ?>one</bravo></alpha>;
y = <alpha><?two bar="foo" ?><bravo><?one foo="bar" ?>one</bravo></alpha>;
TEST(14, false, (x == y) || (y == x));

one = x.*[0];
two = y.*[0];
TEST(15, false, (one == two) || (two == one));

one = x.*[0];
two = y.bravo.*[0];
TEST(16, true, (one == two) && (two == one));

// Namepaces
x = <ns1:alpha xmlns:ns1="http://foo/"><ns1:bravo>one</ns1:bravo></ns1:alpha>;
y = <ns2:alpha xmlns:ns2="http://foo/"><ns2:bravo>one</ns2:bravo></ns2:alpha>;

TEST(17, true, (x == y) && (y == x));

y = <ns2:alpha xmlns:ns2="http://foo"><ns2:bravo>one</ns2:bravo></ns2:alpha>;
TEST(18, false, (x == y) || (y == x));

// Default namespace
default xml namespace = "http://foo/";
x = <alpha xmlns="http://foo/">one</alpha>;
y = <alpha>one</alpha>;

TEST(19, true, (x == y) && (y == x));

// bug 358183
x = <a b="1"/>;
y = <a b="1" c="2"/>;
TEST(20, false,(x == y));

END();
