





var BUGNUMBER = 630770;
var summary =
  'Correctly warn about duplicate parameters when the strict option is enabled';

print(BUGNUMBER + ": " + summary);










options("strict");
eval("function a(x, x, x, x) { }");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
