





var BUGNUMBER = 650574;
var summary = 'Check for too-deep stack when converting a value to source';

print(BUGNUMBER + ": " + summary);





try
{
  var e = Error('');
  e.fileName = e;
  e.toSource();
  throw new Error("should have thrown");
}
catch (e)
{
  assertEq(e instanceof InternalError, true,
           "should have thrown for over-recursion");
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
