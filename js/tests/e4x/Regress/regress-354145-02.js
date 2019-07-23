





































gTestfile = 'regress-354145-02.js';

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

list.child({ toString: prepare });

TEST(1, expect, actual);

END();
