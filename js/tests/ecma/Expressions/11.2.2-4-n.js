





































gTestfile = '11.2.2-4-n.js';












































var SECTION = "11.2.2-4-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var STRING = "";

DESCRIPTION = "STRING = '', var s = new STRING()";
EXPECTED = "error";

new TestCase( SECTION,
	      "STRING = '', var s = new STRING()",
	      "error",
	      eval("s = new STRING()") );
test();

function TestFunction() {
  return arguments;
}
