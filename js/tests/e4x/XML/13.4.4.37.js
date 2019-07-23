









































gTestfile = '13.4.4.37.js';

START("13.4.4.37 - XML text()");

TEST(1, true, XML.prototype.hasOwnProperty("text"));

x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        <bravo>two</bravo>
    </charlie>
</alpha>;

TEST_XML(2, "one", x.bravo.text());

correct = new XMLList();
correct += new XML("one");
correct += new XML("two");
TEST(3, correct, x..bravo.text());   
TEST_XML(4, "", x.charlie.text());
TEST_XML(5, "", x.foobar.text());
TEST_XML(6, "one", x.*.text());

END();