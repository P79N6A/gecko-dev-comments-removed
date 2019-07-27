




var gTestfile = "for-of-var-with-initializer.js";
var BUGNUMBER = 1164741;
var summary = "Don't assert parsing |for (var x = 3 of 42);|";

print(BUGNUMBER + ": " + summary);





try
{
  Function("for (var x = 3 of 42);");
  throw new Error("didn't throw");
}
catch (e)
{
  assertEq(e instanceof SyntaxError, true,
           "expected syntax error, got: " + e);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
