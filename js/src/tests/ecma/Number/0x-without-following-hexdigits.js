



var BUGNUMBER = 582643;
var summary = "'0x' not followed by hex digits should be a syntax error";

print(BUGNUMBER + ": " + summary);





try
{
  eval("0x");
  throw new Error("didn't throw parsing 0x (with no subsequent hex digits)");
}
catch (e)
{
  assertEq(e instanceof SyntaxError, true,
           "bad exception thrown: " + e);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
