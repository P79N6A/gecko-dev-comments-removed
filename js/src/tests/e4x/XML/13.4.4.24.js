









































gTestfile = '13.4.4.24.js';

START("13.4.4.24 - XML namespaceDeclarations()");

TEST(1, true, XML.prototype.hasOwnProperty("namespaceDeclarations"));
   
x =
<foo:alpha xmlns:foo="http://foo/" xmlns:bar="http://bar/">
    <foo:bravo>one</foo:bravo>
</foo:alpha>;

y = x.namespaceDeclarations();

TEST(2, 2, y.length);
TEST(3, "object", typeof(y[0]));
TEST(4, "object", typeof(y[1]));
TEST(5, "foo", y[0].prefix);
TEST(6, Namespace("http://foo/"), y[0]);
TEST(7, "bar", y[1].prefix);
TEST(8, Namespace("http://bar/"), y[1]);

END();