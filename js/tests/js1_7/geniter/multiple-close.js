




































var gTestfile = 'multiple-close.js';

var BUGNUMBER     = "(none)";
var summary = "calling it.close multiple times is harmless";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





function fib()
{
  yield 0; 
  yield 1; 
  yield 1; 
  yield 2; 
  yield 3; 
  yield 5; 
  yield 8; 
}

var failed = false;
var it = fib();

try
{
  if (it.next() != 0)
    throw "0 failed";

  
  it.close();
  it.close();
  it.close();

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
    throw "a closed iterator throws StopIteration on next";
}
catch (e)
{
  failed = e;
}



expect = false;
actual = failed;

reportCompare(expect, actual, summary);
