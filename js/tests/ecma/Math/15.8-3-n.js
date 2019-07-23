





































gTestfile = '15.8-3-n.js';


























var SECTION = "15.8-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Math Object";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "MYMATH = Math()";
EXPECTED = "error";

new TestCase( SECTION,
	      "MYMATH = Math()",
	      "error",
	      eval("MYMATH = Math()") );

test();
