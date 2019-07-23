




































var gTestfile = 'throw-after-close.js';

var BUGNUMBER     = "(none)";
var summary = "gen.close(); gen.throw(ex) throws ex forever";
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
  it.close();

  
  var doThrow = true;
  var thrown = "foobar";
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

  
  doThrow = true;
  thrown = "sparky";
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
    throw "invalid or incorrect StopIteration";
}
catch (e)
{
  failed = e;
}



expect = false;
actual = failed;

reportCompare(expect, actual, summary);
