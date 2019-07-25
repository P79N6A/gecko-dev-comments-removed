











































START("13.4.4.36 - setNamespace");

TEST(1, true, XML.prototype.hasOwnProperty("setNamespace"));

x =
<foo:alpha xmlns:foo="http://foo/" xmlns:bar="http://bar/">
    <foo:bravo>one</foo:bravo>
</foo:alpha>;

correct =
<bar:alpha xmlns:foo="http://foo/" xmlns:bar="http://bar/">
    <foo:bravo>one</foo:bravo>
</bar:alpha>;

x.setNamespace("http://bar/");  

TEST(2, correct, x);

var xhtml1NS = new Namespace('http://www.w3.org/1999/xhtml');
var xhtml = <html />;
xhtml.setNamespace(xhtml1NS);

TEST(3, 1, xhtml.namespaceDeclarations().length);

TEST(4, xhtml1NS, xhtml.namespace());

var xml = <root xmlns:ns="http://example.org/"><blah/></root>
var ns = new Namespace('ns','http://example.org/');
xml.blah.@foo = 'bar';
xml.blah.@foo.setNamespace(ns);
xml.blah.@foo = 'baz';
xml.blah.@foo.setNamespace(ns);

var expected = <root xmlns:ns="http://example.org/"><blah ns:foo="baz"/></root>;

TEST(5, xml, expected);

END();
