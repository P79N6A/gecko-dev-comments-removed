









































gTestfile = '13.4.4.9.js';

START("13.4.4.9 - XML comments()");

TEST(1, true, XML.prototype.hasOwnProperty("comments"));

XML.ignoreComments = false;

x =
<alpha>
    <!-- comment one -->
    <bravo>
    <!-- comment two -->
    some text
    </bravo>
</alpha>;   

TEST_XML(2, "<!-- comment one -->", x.comments());
TEST_XML(3, "<!-- comment two -->", x..*.comments());

END();