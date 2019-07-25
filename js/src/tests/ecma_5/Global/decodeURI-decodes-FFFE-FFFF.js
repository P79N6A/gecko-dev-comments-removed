



var BUGNUMBER = 520095;
var summary =
  "decodeURI{,Component} should return the specified character for " +
  "'%EF%BF%BE' and '%EF%BF%BF', not return U+FFFD";

print(BUGNUMBER + ": " + summary);





assertEq(decodeURI("%EF%BF%BE"), "\uFFFE");
assertEq(decodeURI("%EF%BF%BF"), "\uFFFF");
assertEq(decodeURIComponent("%EF%BF%BE"), "\uFFFE");
assertEq(decodeURIComponent("%EF%BF%BF"), "\uFFFF");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
