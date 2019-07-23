








































gTestfile = 'regress-263934.js';

START("Testing that replacing a list item with a new list that contains that item works");
printBugNumber(263934);

var x =
<x>
  <b>two</b>
  <b>three</b>
</x>;


x.b[0] = <a>one</a> + x.b[0];

var expected =
<x>
  <a>one</a>
  <b>two</b>
  <b>three</b>
</x>;


TEST(1, expected, x);

END();