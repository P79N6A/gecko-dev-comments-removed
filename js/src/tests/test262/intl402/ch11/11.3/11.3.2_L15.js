









$INCLUDE("testBuiltInObject.js");

testBuiltInObject(Object.getOwnPropertyDescriptor(Intl.NumberFormat.prototype, "format").get , true, false, [], 0);

