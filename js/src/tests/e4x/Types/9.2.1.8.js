









































gTestfile = '9.2.1.8.js';

START("9.2.1.8 XMLList [[Descendants]]");

x =
<>
<alpha><bravo>one</bravo></alpha>
<charlie><delta><bravo>two</bravo></delta></charlie>
</>;

correct = <><bravo>one</bravo><bravo>two</bravo></>;
TEST(1, correct, x..bravo);

END();
