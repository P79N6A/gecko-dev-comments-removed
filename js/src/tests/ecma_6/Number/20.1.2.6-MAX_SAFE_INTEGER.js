




var BUGNUMBER = 885798;
var summary = "ES6 (draft April 2014) 20.1.2.6 Number.MAX_SAFE_INTEGER";

print(BUGNUMBER + ": " + summary);






assertEq(Number.MAX_SAFE_INTEGER, Math.pow(2, 53) - 1);


var descriptor = Object.getOwnPropertyDescriptor(Number, 'MAX_SAFE_INTEGER');

assertEq(descriptor.writable, false);
assertEq(descriptor.configurable, false);
assertEq(descriptor.enumerable, false);

if (typeof reportCompare === "function")
  reportCompare(true, true);
