




var BUGNUMBER = 501739;
var summary =
  "String.prototype.replace should throw when called with a global RegExp " +
  "whose .lastIndex is non-writable";

print(BUGNUMBER + ": " + summary);





var s = '0x2x4x6x8';



var p1 = /x/g;
Object.defineProperty(p1, "lastIndex", { writable: false });

try
{
  s.replace(p1, '');
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
  assertEq(p1.lastIndex, 0);
}



var p2 = /x/g;
Object.defineProperty(p2, "lastIndex", { writable: false, value: 3 });

try
{
  s.replace(p2, '');
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
  assertEq(p2.lastIndex, 3);
}



var p3 = /x/g;
Object.defineProperty(p3, "lastIndex", { writable: false });

try
{
  s.replace(p3, 'y');
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
  assertEq(p3.lastIndex, 0);
}



var p4 = /x/g;
Object.defineProperty(p4, "lastIndex", { writable: false, value: 3 });

try
{
  s.replace(p4, '');
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
  assertEq(p4.lastIndex, 3);
}



var p5 = /q/g;
Object.defineProperty(p5, "lastIndex", { writable: false });

try
{
  s.replace(p5, 'y');
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
  assertEq(p5.lastIndex, 0);
}



var p6 = /q/g;
Object.defineProperty(p6, "lastIndex", { writable: false, value: 3 });

try
{
  s.replace(p6, '');
  throw "didn't throw";
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "should have thrown a TypeError, instead got: " + e);
  assertEq(p6.lastIndex, 3);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
