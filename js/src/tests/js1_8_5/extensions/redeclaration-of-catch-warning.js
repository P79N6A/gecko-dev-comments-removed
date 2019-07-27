





var BUGNUMBER = 622646;
var summary = "Shadowing an exception identifier in a catch block with a " +
              "|const| declaration should throw an error.";

print(BUGNUMBER + ": " + summary);





options("strict");
try
{
  evaluate("try {} catch(e) { const e; }");
  throw new Error("Redeclaration error wasn't thrown");
}
catch (e)
{
  assertEq(e.message.indexOf("catch") > 0, true,
           "wrong error, got " + e.message);
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
