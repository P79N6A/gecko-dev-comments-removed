



var BUGNUMBER = 601307;
var summary = "with (...) eval(...) is a direct eval";

print(BUGNUMBER + ": " + summary);





var t = "global";
function test()
{
  var t = "local";
  with ({})
    return eval("t");
}
assertEq(test(), "local");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
