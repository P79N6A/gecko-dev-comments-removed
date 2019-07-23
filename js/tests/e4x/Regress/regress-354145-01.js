





































var bug = 354145;
var summary = 'Immutable XML';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var xml = <></>
var N = 10;
for (var i = 0; i != N; ++i) {
  xml[i] = <{"a"+i}/>;
}

function prepare()
{
  for (var i = N - 1; i >= 0; --i)
    delete xml[i];
  gc();
  return "test";
}

xml[N - 1] = { toString: prepare };

TEST(1, expect, actual);

END();
