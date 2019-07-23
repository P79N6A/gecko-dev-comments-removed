








































START("Testing replacing an element with a list that contains a text node");
BUG(263936);

var x = 
<x>
  <a>one</a>
  <b>three</b>
</x>;


x.a += XML("two");

var expected =
<x>
  <a>one</a>
  two
  <b>three</b>
</x>;

TEST(1, expected, x);

END();