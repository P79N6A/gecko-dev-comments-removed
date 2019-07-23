









































gTestfile = '13.2.2.js';

START("13.2.2 - Namespace Constructor");

n = new Namespace();
TEST(1, "object", typeof(n));
TEST(2, "", n.prefix);
TEST(3, "", n.uri);

n = new Namespace("");
TEST(4, "object", typeof(n));
TEST(5, "", n.prefix);
TEST(6, "", n.uri);

n = new Namespace("http://foobar/");
TEST(7, "object", typeof(n));
TEST(8, "undefined", typeof(n.prefix));
TEST(9, "http://foobar/", n.uri);


m = new Namespace(n);
TEST(10, typeof(n), typeof(m));
TEST(11, n.prefix, m.prefix);
TEST(12, n.uri, m.uri);

n = new Namespace("foobar", "http://foobar/");
TEST(13, "object", typeof(n));
TEST(14, "foobar", n.prefix);
TEST(15, "http://foobar/", n.uri);


m = new Namespace(n);
TEST(16, typeof(n), typeof(m));
TEST(17, n.prefix, m.prefix);
TEST(18, n.uri, m.uri);

try {
    n = new Namespace("ns", "");
    SHOULD_THROW(19);
} catch(ex) {
    TEST(19, "TypeError", ex.name);
}

END();