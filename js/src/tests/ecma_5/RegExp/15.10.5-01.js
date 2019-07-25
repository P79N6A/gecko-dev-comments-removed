




var BUGNUMBER = 614603;
var summary = "RegExp.length";

print(BUGNUMBER + ": " + summary);





assertEq(RegExp.length, 2);
assertEq(/a/.constructor.length, 2);

if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
