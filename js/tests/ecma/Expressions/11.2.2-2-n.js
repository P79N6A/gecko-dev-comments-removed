





































gTestfile = '11.2.2-2-n.js';












































var SECTION = "11.2.2-2-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var UNDEFINED = void 0;

DESCRIPTION = "UNDEFINED = void 0; var o = new UNDEFINED()";
EXPECTED = "error";

new TestCase( SECTION,
	      "UNDEFINED = void 0; var o = new UNDEFINED()",
	      "error",
	      eval("o = new UNDEFINED()") );
test();

function TestFunction() {
  return arguments;
}
