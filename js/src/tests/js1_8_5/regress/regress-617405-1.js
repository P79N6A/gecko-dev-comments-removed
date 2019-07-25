





function C(){}
C.prototype = 1;
assertEq(Object.getOwnPropertyDescriptor(C, "prototype").configurable, false);

reportCompare(0, 0, "ok");
