





































gTestfile = '11.2.2-9-n.js';












































var SECTION = "11.2.2-9-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var BOOLEAN = new Boolean();

DESCRIPTION = "var BOOLEAN = new Boolean(); var b = new BOOLEAN()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var BOOLEAN = new Boolean(); var b = new BOOLEAN()",
	      "error",
	      eval("b = new BOOLEAN()") );
test();

function TestFunction() {
  return arguments;
}
