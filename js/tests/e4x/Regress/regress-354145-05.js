





































gTestfile = 'regress-354145-05.js';

var BUGNUMBER = 354145;
var summary = 'Immutable XML';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var xml = <tag/>;
var N = 10;
for (var i = 0; i != N; ++i)
  xml.appendChild(<{'a'+i}/>);

function prepare()
{
  delete xml.*;
  gc();
  return "test";
}

var last = xml.*[N - 1];
xml.insertChildBefore(last, { toString: prepare });

TEST(1, expect, actual);

END();
