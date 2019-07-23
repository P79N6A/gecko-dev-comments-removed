





































gTestfile = 'regress-354145-03.js';

var BUGNUMBER = 354145;
var summary = 'Immutable XML';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var list = <></>
var N = 10;
for (var i = 0; i != N; ++i)
  list[i] = <{"a"+i}/>;

function prepare()
{
  for (var i = N - 1; i >= 0; --i)
    delete list[i];
  gc();
  return "test";
}

print("Before list.contains");
list.contains({ toString: prepare });
print("After list.contains");
TEST(1, expect, actual);
print("After TEST");

END();
