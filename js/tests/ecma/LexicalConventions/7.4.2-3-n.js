





































gTestfile = '7.4.2-3-n.js';

























var SECTION = "7.4.2-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var new = true";
EXPECTED = "error";

new TestCase( SECTION,  "var new = true",     "error",    eval("var new = true") );

test();
