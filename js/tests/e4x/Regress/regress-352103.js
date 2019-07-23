





































var bug     = "352103";
var summary = "<??> XML initializer should generate a SyntaxError";
var actual, expect;

printBugNumber(bug);
printStatus(summary);





var failed = false;

try
{
  try
  {
    eval("var x = <??>;"); 
    throw "No SyntaxError thrown!";
  }
  catch (e)
  {
    if (!(e instanceof SyntaxError))
      throw "Unexpected exception: " + e;
  }
}
catch (ex)
{
  failed = ex;
}

expect = false;
actual = failed;

TEST(1, expect, actual);
