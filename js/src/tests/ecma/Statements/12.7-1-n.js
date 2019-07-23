





































gTestfile = '12.7-1-n.js';









var SECTION = "12.7.1-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The continue statement";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "continue";
EXPECTED = "error";

new TestCase(   SECTION,
		"continue",
		"error",
		eval("continue") );

test();
