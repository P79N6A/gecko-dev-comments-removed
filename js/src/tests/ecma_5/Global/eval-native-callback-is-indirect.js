



var BUGNUMBER = 604504;
var summary = "eval called from a native function is indirect";

print(BUGNUMBER + ": " + summary);





var originalEval = eval;

var global = this;
var directCheckCode = "this === global";

function testBound()
{
  var global = "psych!";
  var eval = originalEval.bind(undefined, directCheckCode);
  assertEq(eval(), true);
}
testBound();



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
