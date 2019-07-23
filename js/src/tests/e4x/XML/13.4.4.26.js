










































gTestfile = '13.4.4.26.js';

START("13.4.4.26 - XML normalize()");

TEST(1, true, XML.prototype.hasOwnProperty("normalize"));

XML.ignoreWhitespace = false;
XML.prettyPrinting = false;

var x = <alpha> <bravo> one </bravo> </alpha>;

TEST_XML(2, "<alpha> <bravo> one </bravo> </alpha>", x);
x.normalize();
TEST_XML(3, "<alpha> <bravo> one </bravo> </alpha>", x);




delete x.bravo[0];

TEST_XML(4, "<alpha>  </alpha>", x);
TEST(5, 2, x.children().length());

x.normalize();

TEST_XML(6, "<alpha>  </alpha>", x);
TEST(7, 1, x.children().length());



x.appendChild(<bravo> fun </bravo>);

TEST_XML(8, "<alpha>  <bravo> fun </bravo></alpha>", x);
TEST(9, 2, x.children().length());



var y = <charlie> <delta/> </charlie>;

TEST(10, 3, y.children().length());

x.appendChild(y);
delete y.delta[0];

TEST(11, 2, y.children().length());

x.normalize();

TEST(12, 1, y.children().length());
TEST(13, 1, x.charlie.children().length());





x = <alpha><beta/></alpha>;

TEST_XML(14, "<alpha><beta/></alpha>", x);
TEST(15, 1, x.children().length());

x.appendChild(XML());

TEST_XML(16, "<alpha><beta/></alpha>", x);
TEST(17, 2, x.children().length());

x.normalize();

TEST_XML(18, "<alpha><beta/></alpha>", x);
TEST(19, 1, x.children().length());

x.appendChild(XML(" "));

TEST_XML(20, "<alpha><beta/> </alpha>", x);
TEST(21, 2, x.children().length());

x.normalize();


TEST_XML(22, "<alpha><beta/> </alpha>", x);
TEST(23, 2, x.children().length());

y = <foo/>;
y.appendChild(XML());

TEST(24, 1, y.children().length());

x.appendChild(y);



x.normalize();

TEST(25, 0, y.children().length());

END();
