





































gTestfile = '12.6.2-9-n.js';















var SECTION = "12.6.2-9-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for statement";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "for (i)";
EXPECTED = "error";

new TestCase( SECTION,
	      "for (i)",
	      "error",
	      eval("for (i) { }") );






test();

