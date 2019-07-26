




var BUGNUMBER = 843004;
var summary =
  "Use of an object that emulates |undefined| as the sole option must " +
  "preclude imputing default values";

print(BUGNUMBER + ": " + summary);





var opt = objectEmulatingUndefined();
opt.toString = function() { return "long"; };

var str = new Date(2013, 12 - 1, 14).toLocaleString("en-US", { weekday: opt });



assertEq(str, "Saturday");

if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
