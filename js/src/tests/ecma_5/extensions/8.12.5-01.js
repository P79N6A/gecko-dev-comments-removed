









var BUGNUMBER = 523846;
var summary =
  "Assignments to a property that has a getter but not a setter should not " +
  "throw a TypeError per ES5 (at least not until strict mode is supported)";
var actual = "Early failure";
var expect = "No errors";


printBugNumber(BUGNUMBER);
printStatus(summary);

var o = { get p() { return "a"; } };

function test1()
{
  o.p = "b"; 
  assertEq(o.p, "a");
}

function test2()
{
  function T() {}
  T.prototype = o;
  y = new T();
  y.p = "b";  
  assertEq(y.p, "a");
}


var errors = [];
try
{
  try
  {
    test1();
  }
  catch (e)
  {
    errors.push(e);
  }

  try
  {
    test2();
  }
  catch (e)
  {
    errors.push(e);
  }

  options("strict");
  options("werror");
  try
  {
    test1();
    errors.push("strict+werror didn't make test1 fail");
  }
  catch (e)
  {
    if (!(e instanceof TypeError))
      errors.push("test1 with strict+werror failed without a TypeError: " + e);
  }

  try
  {
    test2();
    errors.push("strict+werror didn't make test2 fail");
  }
  catch (e)
  {
    if (!(e instanceof TypeError))
      errors.push("test2 with strict+werror failed without a TypeError: " + e);
  }

  options("strict");
  options("werror");
}
catch (e)
{
  errors.push("Unexpected error: " + e);
}
finally
{
  actual = errors.length > 0 ? errors.join(", ") : "No errors";
}

reportCompare(expect, actual, summary);
