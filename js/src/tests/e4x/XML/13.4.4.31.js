









































gTestfile = '13.4.4.31.js';

START("13.4.4.31 - XML removeNamespace()");

TEST(1, true, XML.prototype.hasOwnProperty("removeNamespace"));

x =
<alpha xmlns:foo="http://foo/">
    <bravo>one</bravo>
</alpha>;

correct =
<alpha>
    <bravo>one</bravo>
</alpha>;

x.removeNamespace("http://foo/");

TEST(2, correct, x);


x =
<foo:alpha xmlns:foo="http://foo/">
    <bravo>one</bravo>
</foo:alpha>;

correct =
<foo:alpha xmlns:foo="http://foo/">
    <bravo>one</bravo>
</foo:alpha>;

x.removeNamespace("http://foo/");

TEST(3, correct, x);

END();