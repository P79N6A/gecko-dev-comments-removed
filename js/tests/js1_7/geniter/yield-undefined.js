




































var gTestfile = 'yield-undefined.js';




var BUGNUMBER     = "(none)";
var summary = "|yield;| is equivalent to |yield undefined;| ";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

function gen()
{
  yield 7;
  yield;
  yield 3;
}

var it = gen();

try
{
  if (it.next() != 7)
    throw "7 not yielded";
  if (it.next() !== undefined)
    throw "|yield;| should be equivalent to |yield undefined;|";
  if (it.next() != 3)
    throw "3 not yielded";

  var stopPassed = false;
  try
  {
    it.next();
  }
  catch (e)
  {
    if (e === StopIteration)
      stopPassed = true;
  }

  if (!stopPassed)
    throw "it: missing or incorrect StopIteration";
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
