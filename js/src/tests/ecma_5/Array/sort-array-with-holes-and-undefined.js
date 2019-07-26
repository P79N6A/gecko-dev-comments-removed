





var BUGNUMBER = 664528;
var summary =
  "Sorting an array containing only holes and |undefined| should move all " +
  "|undefined| to the start of the array";

print(BUGNUMBER + ": " + summary);





var a = [, , , undefined];
a.sort();

assertEq(a.hasOwnProperty(0), true);
assertEq(a[0], undefined);
assertEq(a.hasOwnProperty(1), false);
assertEq(a.hasOwnProperty(2), false);
assertEq(a.hasOwnProperty(3), false);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
