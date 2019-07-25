






var gTestfile = 'preventExtensions-idempotent.js';

var BUGNUMBER = 599459;
var summary = 'Object.preventExtensions should be idempotent';

print(BUGNUMBER + ": " + summary);





var obj = {};
assertEq(Object.preventExtensions(obj), obj);
assertEq(Object.isExtensible(obj), false);
assertEq(Object.preventExtensions(obj), obj);
assertEq(Object.isExtensible(obj), false);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
