




var BUGNUMBER = 501739;
var summary =
  "String.prototype.match should throw when called with a global RegExp " +
  "whose .lastIndex is non-writable";

print(BUGNUMBER + ": " + summary);





var s = '0x2x4x6x8';



var p1 = /x/g;
Object.defineProperty(p1, "lastIndex", { writable: false });

try
{
  s.match(p1);
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
}



var p2 = /x/g;
Object.defineProperty(p2, "lastIndex", { writable: false, value: 3 });

try
{
  s.match(p2);
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
}



var p3 = /q/g;
Object.defineProperty(p3, "lastIndex", { writable: false });

try
{
  s.match(p3);
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
}



var p4 = /q/g;
Object.defineProperty(p4, "lastIndex", { writable: false, value: 3 });

try
{
  s.match(p4);
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
