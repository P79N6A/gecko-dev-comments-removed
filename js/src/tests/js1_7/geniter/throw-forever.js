




































var gTestfile = 'throw-forever.js';

var BUGNUMBER     = "(none)";
var summary = "gen.throw(ex) returns ex for an exhausted gen";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





function gen()
{
  var x = 5, y = 7;
  var z = x + y;
  yield z;
}

var failed = false;
var it = gen();

try
{
  
  var thrown = "foobar";
  var doThrow = true;
  try
  {
    it.throw(thrown);
  }
  catch (e)
  {
    if (e === thrown)
      doThrow = false;
  }
  if (doThrow)
    throw "it.throw(\"" + thrown + "\") failed";

  
  
  thrown = "baz";
  doThrow = true;
  try
  {
    it.throw(thrown);
  }
  catch (e)
  {
    if (e === thrown)
      doThrow = false;
  }
  if (doThrow)
    throw "it.throw(\"" + thrown + "\") failed";

  
  
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
    throw "missing or incorrect StopIteration";
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
