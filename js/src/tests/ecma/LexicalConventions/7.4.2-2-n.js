





































gTestfile = '7.4.2-2-n.js';

























var SECTION = "7.4.1-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var for = true";
EXPECTED = "error";

new TestCase( SECTION,  "var for = true",     "error",    eval("var for = true") );

test();
