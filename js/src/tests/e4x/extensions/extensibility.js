






var gTestfile = 'extensibility.js';

var BUGNUMBER = 492849;
var summary = 'XML values cannot have their [[Extensible]] property changed';

print(BUGNUMBER + ": " + summary);





var x = <foo/>;

assertEq(Object.isExtensible(x), true);

try
{
  Object.preventExtensions(x);
  throw new Error("didn't throw");
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "xmlValue.[[Extensible]] cannot be changed");
}

assertEq(Object.isExtensible(x), true);




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
