





var BUGNUMBER     = "(none)";
var summary = "A (slow) generator of pi";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





function pi()
{
  var val = 0;
  var curr = 1;
  var isNeg = false;
  while (true)
  {
    if (isNeg)
      yield val -= 4/curr;
    else
      yield val += 4/curr;
    curr += 2;
    isNeg = !isNeg;
  }
}

var failed = false;
var it = pi();

var vals =
  [4,
   4 - 4/3,
   4 - 4/3 + 4/5,
   4 - 4/3 + 4/5 - 4/7];

try
{
  for (var i = 0, sz = vals.length; i < sz; i++)
    if (it.next() != vals[i])
      throw vals[i];
}
catch (e)
{
  failed = e;
}



expect = false;
actual = failed;

reportCompare(expect, actual, summary);
