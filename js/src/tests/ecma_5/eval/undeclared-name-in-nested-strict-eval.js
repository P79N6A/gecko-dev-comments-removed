

"use strict";


var BUGNUMBER = 514568;
var summary =
  "Verify that we don't optimize free names to gnames in eval code that's " +
  "global, when the name refers to a binding introduced by a strict mode " +
  "eval frame";

print(BUGNUMBER + ": " + summary);





var x = "global";
assertEq(eval('var x = "eval"; eval("x")'), "eval");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete!");
