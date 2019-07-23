





































gTestfile = 'toString-001-n.js';










var SECTION = "toString-001.js";
var VERSION = "JS1_4";
var TITLE   = "Regression test case for 310514";
var BUGNUMBER="310514";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

var o = {};
o.toString = Function.prototype.toString;

DESCRIPTION = "var o = {}; o.toString = Function.prototype.toString; o.toString();";
EXPECTED = "error";

new TestCase(
  SECTION,
  "var o = {}; o.toString = Function.prototype.toString; o.toString();",
  "error",
  o.toString() );

test();
