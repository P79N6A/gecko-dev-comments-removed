









































gTestfile = '10.3.js';

START("10.3 - toXML");

var x;


x = new Boolean(true);
TEST_XML(1, "true", new XML(x));


x = new Number(123);
TEST_XML(2, "123", new XML(x));


x = new String("<alpha><bravo>one</bravo></alpha>");
TEST(3, <alpha><bravo>one</bravo></alpha>, new XML(x));


x = new XML(<alpha><bravo>one</bravo></alpha>);
TEST(4, <alpha><bravo>one</bravo></alpha>, new XML(x));


x = new XMLList(<alpha><bravo>one</bravo></alpha>);
TEST(5, <alpha><bravo>one</bravo></alpha>, new XML(x));

try {
    x = new XMLList(<alpha>one</alpha> + <bravo>two</bravo>);
    new XML(x);
    SHOULD_THROW(6);
} catch (ex) {
    TEST(6, "TypeError", ex.name);
}

END();
