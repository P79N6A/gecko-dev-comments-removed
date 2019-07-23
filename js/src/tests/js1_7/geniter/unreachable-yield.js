




































var gTestfile = 'unreachable-yield.js';

var BUGNUMBER     = "(none)";
var summary = "Iterator with unreachable yield statement";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





function gen()
{
  
  
  
  if (false)
    yield "failed";
}

var failed = false;
try
{
  var it = gen();
  if (it == undefined)
    throw "gen() not recognized as generator";

  
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
    throw "incorrect or invalid StopIteration";
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
