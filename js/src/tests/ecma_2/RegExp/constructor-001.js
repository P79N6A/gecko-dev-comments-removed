













var SECTION = "RegExp/constructor-001";
var VERSION = "ECMA_2";
var TITLE   = "new RegExp()";

startTest();












RegExp.prototype.getClassProperty = Object.prototype.toString;
var re = new RegExp();

AddTestCase(
  "RegExp.prototype.getClassProperty = Object.prototype.toString; " +
  "(new RegExp()).getClassProperty()",
  "[object RegExp]",
  re.getClassProperty() );

AddTestCase(
  "(new RegExp()).source",
  "(?:)",
  re.source );

AddTestCase(
  "(new RegExp()).global",
  false,
  re.global );

AddTestCase(
  "(new RegExp()).ignoreCase",
  false,
  re.ignoreCase );

AddTestCase(
  "(new RegExp()).multiline",
  false,
  re.multiline );

AddTestCase(
  "(new RegExp()).lastIndex",
  0,
  re.lastIndex );

test()
