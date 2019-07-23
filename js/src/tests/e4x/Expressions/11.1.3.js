









































gTestfile = '11.1.3.js';

START("11.1.3 - Wildcard Identifiers");

x =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
</alpha>

correct = <><bravo>one</bravo><charlie>two</charlie></>;
TEST(1, correct, x.*);

END();
