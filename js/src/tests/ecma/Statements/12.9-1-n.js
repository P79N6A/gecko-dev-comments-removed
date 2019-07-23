





































gTestfile = '12.9-1-n.js';









var SECTION = "12.9-1-n";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " The return statement");

DESCRIPTION = "return";
EXPECTED = "error";

new TestCase(   SECTION,
		"return",
		"error",
		eval("return") );

test();
