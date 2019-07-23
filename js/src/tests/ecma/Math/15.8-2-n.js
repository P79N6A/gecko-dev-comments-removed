





































gTestfile = '15.8-2-n.js';



























var SECTION = "15.8-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Math Object";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "MYMATH = new Math()";
EXPECTED = "error";

new TestCase( SECTION,
	      "MYMATH = new Math()",
	      "error",
	      eval("MYMATH = new Math()") );

test();
