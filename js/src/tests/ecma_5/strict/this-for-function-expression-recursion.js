




var gTestfile = 'this-for-function-expression-recursion.js';
var BUGNUMBER = 611276;
var summary = "JSOP_CALLEE should push undefined, not null, for this";

print(BUGNUMBER + ": " + summary);









var calleeThisFun =
  function calleeThisFun(recurring)
  {
    if (recurring)
      return this;
    return calleeThisFun(true);
  };
assertEq(calleeThisFun(false), this);

var calleeThisStrictFun =
  function calleeThisStrictFun(recurring)
  {
    "use strict";
    if (recurring)
      return this;
    return calleeThisStrictFun(true);
  };
assertEq(calleeThisStrictFun(false), undefined);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
