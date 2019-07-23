









































gTestfile = '13.2.1.js';

START("13.2.1 - Namespace Constructor as Function");

n = Namespace();
m = new Namespace();
TEST(1, typeof(m), typeof(n));
TEST(2, m.prefix, n.prefix);
TEST(3, m.uri, n.uri);

n = Namespace("http://foobar/");
m = new Namespace("http://foobar/");
TEST(4, typeof(m), typeof(n));
TEST(5, m.prefix, n.prefix);
TEST(6, m.uri, n.uri);

n = Namespace("foobar", "http://foobar/");
m = new Namespace("foobar", "http://foobar/");
TEST(7, typeof(m), typeof(n));
TEST(8, m.prefix, n.prefix);
TEST(9, m.uri, n.uri);

n = Namespace(m);
TEST(10, m, n);

END();