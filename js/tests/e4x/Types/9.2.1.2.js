









































gTestfile = '9.2.1.2.js';

START("9.2.1.2 - XMLList [[Put]]");

var x =
<>
    <alpha>one</alpha>
    <bravo>two</bravo>
</>;

TEST(1, "<alpha>one</alpha>\n<bravo>two</bravo>",
  x.toXMLString());

x[0] = <charlie>three</charlie>;
TEST(2, "<charlie>three</charlie>\n<bravo>two</bravo>",
  x.toXMLString());

x[0] = <delta>four</delta> + <echo>five</echo>;
TEST(3, "<delta>four</delta>\n<echo>five</echo>\n<bravo>two</bravo>",
  x.toXMLString());

END();