









































gTestfile = '13.3.1.js';

START("13.3.1 - QName Constructor as a Function");

q = QName("foobar");
p = new QName("foobar");
TEST(1, typeof(p), typeof(q));
TEST(2, p.localName, q.localName);
TEST(3, p.uri, q.uri);

q = QName("http://foobar/", "foobar");
p = new QName("http://foobar/", "foobar");
TEST(4, typeof(p), typeof(q));
TEST(5, p.localName, q.localName);
TEST(6, p.uri, q.uri);

p1 = QName(q);
p2 = new QName(q);
TEST(7, typeof(p2), typeof(p1));
TEST(8, p2.localName, p1.localName);
TEST(9, p2.uri, p1.uri);

n = new Namespace("http://foobar/");
q = QName(n, "foobar");
p = QName(n, "foobar");
TEST(10, typeof(p), typeof(q));
TEST(11, p.localName, q.localName);
TEST(12, p.uri, q.uri);

p = QName(q);
TEST(13, p, q);

END();