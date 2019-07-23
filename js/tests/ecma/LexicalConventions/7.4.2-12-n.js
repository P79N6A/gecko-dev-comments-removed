





































gTestfile = '7.4.2-12-n.js';

























var SECTION = "7.4.1-12-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var while = true";
EXPECTED = "error";

new TestCase( SECTION,  "var while = true",     "error",    eval("var while = true") );

test();
