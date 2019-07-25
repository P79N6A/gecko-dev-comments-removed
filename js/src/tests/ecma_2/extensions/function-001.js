













var SECTION = "RegExp/function-001";
var VERSION = "ECMA_2";
var TITLE   = "RegExp( pattern, flags )";

startTest();












RegExp.prototype.getClassProperty = Object.prototype.toString;
var re = new RegExp();

AddTestCase(
  "new RegExp().__proto__",
  RegExp.prototype,
  re.__proto__
  );

test()
