





































gTestfile = '11.2.2-1-n.js';












































var SECTION = "11.2.2-1-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var OBJECT = new Object();

DESCRIPTION = "OBJECT = new Object; var o = new OBJECT()";
EXPECTED = "error";

new TestCase( SECTION,
	      "OBJECT = new Object; var o = new OBJECT()",
	      "error",
	      eval("o = new OBJECT()") );
test();

function TestFunction() {
  return arguments;
}
