





































gTestfile = 'regress-352103.js';


var BUGNUMBER     = "352103";
var summary = "<??> XML initializer should generate a SyntaxError";
var actual, expect;

printBugNumber(BUGNUMBER);
START(summary);





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
