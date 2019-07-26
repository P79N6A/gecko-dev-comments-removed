









$INCLUDE("testBuiltInObject.js");

testBuiltInObject(Object.getOwnPropertyDescriptor(Intl.Collator.prototype, "compare").get , true, false, [], 0);

