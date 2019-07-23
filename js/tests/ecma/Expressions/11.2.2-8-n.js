





































gTestfile = '11.2.2-8-n.js';












































var SECTION = "11.2.2-8-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var NUMBER = new Number(1);

DESCRIPTION = "var NUMBER = new Number(1); var n = new NUMBER()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var NUMBER = new Number(1); var n = new NUMBER()",
	      "error",
	      eval("n = new NUMBER()") );
test();

function TestFunction() {
  return arguments;
}
