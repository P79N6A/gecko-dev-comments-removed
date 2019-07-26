





var BUGNUMBER = 858381;
var summary = 'Object.freeze([]).pop() must throw a TypeError';

print(BUGNUMBER + ": " + summary);





try
{
  Object.freeze([]).pop();
  throw new Error("didn't throw");
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown TypeError, instead got: " + e);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
