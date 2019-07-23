





































gTestfile = '15.1-2-n.js';













var SECTION = "15.1-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Global Object";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var MY_GLOBAL = this()";
EXPECTED = "error";

new TestCase(   SECTION,
		"var MY_GLOBAL = this()",
		"error",
		eval("var MY_GLOBAL = this()") );
test();
