


var gTestfile = 'stringify-missing-arguments.js';

var BUGNUMBER = 648471;
var summary = "JSON.stringify with no arguments";

print(BUGNUMBER + ": " + summary);





assertEq(JSON.stringify(), undefined);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
