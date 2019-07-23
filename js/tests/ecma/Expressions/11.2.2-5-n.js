





































gTestfile = '11.2.2-5-n.js';












































var SECTION = "11.2.2-5-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var NUMBER = 0;

DESCRIPTION = "NUMBER=0, var n = new NUMBER()";
EXPECTED = "error";

new TestCase( SECTION,
	      "NUMBER=0, var n = new NUMBER()",
	      "error",
	      eval("n = new NUMBER()") );
test();

function TestFunction() {
  return arguments;
}
