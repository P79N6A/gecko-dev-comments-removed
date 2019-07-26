




var BUGNUMBER = 501739;
var summary =
  "String.prototype.match behavior with zero-length matches involving " +
  "forward lookahead";

print(BUGNUMBER + ": " + summary);





var r = /(?=x)/g;

var res = "aaaaaaaaaxaaaaaaaaax".match(r);
assertEq(res.length, 2);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
