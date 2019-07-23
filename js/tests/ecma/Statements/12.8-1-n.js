





































gTestfile = '12.8-1-n.js';










var SECTION = "12.8-1-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The break in statement";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "break";
EXPECTED = "error";

new TestCase(   SECTION,
		"break",
		"error",
		eval("break") );


test();

