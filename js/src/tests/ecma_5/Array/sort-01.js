





var BUGNUMBER = 604971;
var summary = 'array.sort compare-function gets incorrect this';

print(BUGNUMBER + ": " + summary);





[1, 2, 3].sort(function() { "use strict"; assertEq(this, undefined); });



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
