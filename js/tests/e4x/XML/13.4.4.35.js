









































gTestfile = '13.4.4.35.js';

START("13.4.4.35 - setName");

TEST(1, true, XML.prototype.hasOwnProperty("setName"));

x =
<alpha>
    <bravo>one</bravo>
</alpha>;

correct =
<charlie>
    <bravo>one</bravo>
</charlie>;

x.setName("charlie");

TEST(2, correct, x);

x =
<ns:alpha xmlns:ns="http://foobar/">
    <ns:bravo>one</ns:bravo>
</ns:alpha>;

correct =
<charlie xmlns:ns="http://foobar/">
    <ns:bravo>one</ns:bravo>
</charlie>;

x.setName("charlie");

TEST(3, correct, x);

x =
<ns:alpha xmlns:ns="http://foobar/">
    <ns:bravo>one</ns:bravo>
</ns:alpha>;

correct =
<ns:charlie xmlns:ns="http://foobar/">
    <ns:bravo>one</ns:bravo>
</ns:charlie>;

x.setName(new QName("http://foobar/", "charlie"));

TEST(4, correct, x);

END();