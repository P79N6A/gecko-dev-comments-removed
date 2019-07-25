













var SECTION = "RegExp/constructor-001";
var VERSION = "ECMA_2";
var TITLE   = "new RegExp()";

startTest();












RegExp.prototype.getClassProperty = Object.prototype.toString;
var re = new RegExp();

AddTestCase(
  "new RegExp().__proto__",
  RegExp.prototype,
  re.__proto__
  );

test()
