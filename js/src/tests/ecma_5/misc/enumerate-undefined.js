





var BUGNUMBER = 547087;
var summary = 'JS_EnumerateStandardClasses uses wrong attributes for undefined';

print(BUGNUMBER + ": " + summary);





for (var p in this);

assertEq(Object.getOwnPropertyDescriptor(this, "undefined").writable, false);



reportCompare(true, true);

print("All tests passed!");
