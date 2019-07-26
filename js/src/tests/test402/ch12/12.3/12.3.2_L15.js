









$INCLUDE("testBuiltInObject.js");

testBuiltInObject(Object.getOwnPropertyDescriptor(Intl.DateTimeFormat.prototype, "format").get , true, false, [], 0);

