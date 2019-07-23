





































gTestfile = 'function-004-n.js';










var SECTION = "funtion-004-n.js";
var VERSION = "JS1_4";
var TITLE   = "Regression test case for 310502";
var BUGNUMBER="310502";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

var o  = {};
o.call = Function.prototype.call;

DESCRIPTION = "var o = {}; o.call = Function.prototype.call; o.call()";
EXPECTED = "error";

new TestCase(
  SECTION,
  "var o = {}; o.call = Function.prototype.call; o.call()",
  "error",
  o.call() );

test();
