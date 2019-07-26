



var BUGNUMBER = 885798;
var summary = "ES6 (draft May 2013) 15.7.3.7 Number.EPSILON";

print(BUGNUMBER + ": " + summary);






assertEq(Number.EPSILON, Math.pow(2, -52));


var descriptor = Object.getOwnPropertyDescriptor(Number, 'EPSILON');
assertEq(descriptor.writable, false);
assertEq(descriptor.configurable, false);
assertEq(descriptor.enumerable, false);

if (typeof reportCompare === "function")
  reportCompare(true, true);
