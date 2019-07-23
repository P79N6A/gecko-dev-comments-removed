





































gTestfile = '7.4.2-10-n.js';

























var SECTION = "7.4.1-10-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var if = true";
EXPECTED = "error";

new TestCase( SECTION,  "var if = true",     "error",    eval("var if = true") );

test();
