





































gTestfile = '7.4.2-15-n.js';

























var SECTION = "7.4.1-15-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var typeof = true";
EXPECTED = "error";

new TestCase( SECTION,  "var typeof = true",     "error",    eval("var typeof = true") );

test();
