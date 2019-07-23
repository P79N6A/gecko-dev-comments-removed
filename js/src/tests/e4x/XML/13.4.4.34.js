









































gTestfile = '13.4.4.34.js';

START("13.4.4.34 - XML setLocalName()");

TEST(1, true, XML.prototype.hasOwnProperty("setLocalName"));

x =
<alpha>
    <bravo>one</bravo>
</alpha>;

correct =
<charlie>
    <bravo>one</bravo>
</charlie>;

x.setLocalName("charlie");

TEST(2, correct, x);

x =
<ns:alpha xmlns:ns="http://foobar/">
    <ns:bravo>one</ns:bravo>
</ns:alpha>;

correct =
<ns:charlie xmlns:ns="http://foobar/">
    <ns:bravo>one</ns:bravo>
</ns:charlie>;

x.setLocalName("charlie");

TEST(3, correct, x);

END();