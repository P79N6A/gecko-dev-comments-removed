









$INCLUDE("testBuiltInObject.js");

testBuiltInObject(fnGlobalObject().Intl, false, false, []);
testBuiltInObject(Intl, false, false, ["Collator", "NumberFormat", "DateTimeFormat"]);

