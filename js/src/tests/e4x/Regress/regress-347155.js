





































gTestfile = 'regress-347155.js';

var BUGNUMBER = 347155;
var summary = 'Do not crash with deeply nested e4x literal';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

function repeat(str, num)
{
  var s="", i;
  for (i=0; i<num; ++i)
    s += str;
  return s;
}

n = 100000;
e4x = repeat("<x>", n) + 3 + repeat("</x>", n);
try
{
    eval(e4x);
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
